#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#include <poll.h>

#define RCVSIZE 1024
#define TRUE 1
#define FALSE 0
#define TABSIZE 1406
#define FREADSIZE 250	//combien de segments stockés dans le buffer




int envoifile(char* nomf, int descenv2, struct sockaddr_in adresseenv2);
int extractack(char * t);
int getTaille(FILE* fp);
int updateFlightSize(int seg, int numsegrecu);
int getAttente(int rtt){
	if (rtt == 0) {
		return 1.5;
	}
	return rtt;
}

struct timeval end[1000], start[1000];
char renvoitab[30*1406];
double t1,t2;
int LOGS;

int main (int argc, char *argv[]) {
	if(argc<=2){
		printf("ERROR : necessite un argument : appel sous la forme './serveur numport <1 ou 0 pour afficher logs>'\n");
		exit(-1);
	}
	if (atoi(argv[2]) == 1){
		LOGS = 1;
	}else {
		LOGS = 0;
	}
	printf("INFO : etat des logs = %d\n", LOGS);	
	
	struct sockaddr_in serveur;
	int port = atoi(argv[1]);
	int portserv=port;
	int valid= 1;
	char buffer[RCVSIZE];
	char str[4];

	char addrserv[]="192.168.0.50"; //192.168.0.50    ou    192.168.5.298
	printf("INFO : serveur lance sur l' adresse %s\n",addrserv);
	//create socket
	int descserv= socket(AF_INET, SOCK_DGRAM, 0);

	// handle error	
	if (descserv < 0) {
		perror("cannot create socket UDP serveur\n");
		return -1;
	}
	
	setsockopt(descserv, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

	serveur.sin_family= AF_INET;
	serveur.sin_port= htons(portserv);
	serveur.sin_addr.s_addr= htonl(INADDR_ANY);
	
	if (bind(descserv, (struct sockaddr*) &serveur, sizeof(serveur)) == -1) {
		perror("blind descserv fail\n");
		close(descserv);
		return -1;
	}

	fd_set rfds;

	while (1) {
		
           socklen_t taille = sizeof(serveur);
			int read = recvfrom(descserv, buffer, sizeof(buffer), 0,  (struct sockaddr *) &serveur, &taille);
     
			if( read <= 0 )
			{
				perror( "recvfrom() error \n" );
				return -1;
			}
			//printf("INFO : SYN : %s\n",buffer);
			if(strcmp("SYN",buffer)==0){
				printf("INFO : SYN Received\n");
				port++;
				 pid_t pid;
				do {

				pid = fork();

				} while ((pid == -1) && (errno == EAGAIN));

   

				if(pid==0){
					/* Si on est dans le fils */
					struct sockaddr_in client;
					socklen_t taillecli = sizeof(client);
					int desccli= socket(AF_INET, SOCK_DGRAM, 0);
					if (desccli < 0) {
						perror("cannot create socket UDP client\n");
						exit(-1);
					}
					setsockopt(desccli, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
					client.sin_family= AF_INET;
					client.sin_port= htons(port);
					printf("INFO : port utilisé par le client = %d\n",port);
					inet_aton(addrserv,&client.sin_addr);
					if (bind(desccli, (struct sockaddr*) &client, sizeof(client)) == -1) {
						perror("bind descserv2 fail ");
						close(desccli);
						exit(-1);
					}
					FD_SET(desccli, &rfds);
					int tb = 12;
					char buffersyn[tb];
					strcpy(buffersyn, "SYN-ACK");
					sprintf(str, "%d", port);
					strcat(buffersyn, str);
					//printf("buffersynack : %s \n", buffersyn);
					sendto(descserv, buffersyn, sizeof(buffersyn), 0, (struct sockaddr *)&serveur, taille);
					printf("INFO : SYN-ACK sent \n");
					read = recvfrom(descserv, &buffer, sizeof(buffer), 0,  (struct sockaddr *) &serveur, &taille);
     
					if( read <= 0 )
					{
						perror( "recvfrom() error \n" );
						return -1;
					}
					//printf("ACK : %s\n",buffer);
			
			
					if(strcmp("ACK",buffer)==0){
						printf("INFO : ACK Received\n");
						read = recvfrom(desccli, buffer, sizeof(buffer), 0,  (struct sockaddr *) &client, &taillecli);
						if( read <= 0 )
						{
							perror( "recvfrom() error \n" );
							return -1;
						}
						printf("INFO : debut envoi fichier %s\n",buffer);
						envoifile(buffer,desccli, client);
						printf("INFO : fin de la transmission, fermeture du thread\n");
						close(desccli);
						exit(1);
						}else{
							printf("ERROR : ACK invalide : %s\n",buffer);
						}
						close(desccli);
				}else{
					sleep(1);//NE PAS SUPPRIMER
				/* Si on est dans le père */

				}
				
			}else{
				printf("ERROR : message SYN invalide : %s\n", buffer);
			}
	}


return 0;
}



int envoifile(char* nomf, int descenv2, struct sockaddr_in adresseenv2){
		//taille segment 1406
		socklen_t taille = sizeof(adresseenv2);
		int i,cpt,nbelemrttmoy=5;
		char tab[TABSIZE];
		char octets[TABSIZE-6];
		int nbseg, seg=1;
		int taillef=0;
		double rtt=0, rttmoy=(0.03); //valeur de rttmoy pris comme valeur initiale : 50ms. ne compte que pour les premieres trames
		int sstresh = 9999999;
		int duplicateACK=0;
		int window = 1;
		int segaack=1;
		//variable ack
		int numsegrecu=0;
		char bufferrec[RCVSIZE];
		int flightSize =0;
		int windowAcc = 0; //incrémenteur utilisé pour le congestion avoidance
		int nbAckSent=0;
		int lastReceivedACK=0;
		int nbBytes=0;
		int isItEOF = 0;
		int j,k; //utilisés pour le tableau de renvoi
		int segaenv;
		
		int attente;
	
		FILE* f = NULL;
		f = fopen(nomf,"r");
		if (f != NULL){
        	
        	
    	}else{
        	printf("Impossible d'ouvrir le fichier %s\n",nomf);
    	}
		taillef = getTaille(f);
		

		//init pour poll()v2
		int rv;
		struct pollfd fds[1];
  		int    nfds = 1;
		memset(fds, 0 , sizeof(fds));
		fds[0].fd = descenv2;
  		fds[0].events = POLLIN;
		printf("INFO : taille du fichier = %d\n",taillef);

		sprintf(tab, "%d", taillef);
		//calcul du nombre de segments requis
		if(taillef%1400!=0){
			nbseg = (taillef/1400)+1;
		}else{
			nbseg = (taillef/1400);
		}
		i=0;
		
		
//----------- INIT TABLEAU SEGMENTS -------------------------------
		char contenuBuff[FREADSIZE][TABSIZE-6]; //taleau contenant les 200 (default) segments courants
		int contenuBuffSize[FREADSIZE]; //taleau contenant la taille des 200 segments
		int parties = taillef/(FREADSIZE * (TABSIZE-6)); // nombre de parties de 200 segments
		if (parties%taillef != 0){
			parties ++;
		}
		int partiesNonTraitees = parties; //nombre de parties restant a traiter
		int segmentsRestants = nbseg;
		int nbSegInThisPartie; //nombre de segments a envoyer dans la partie courante
		if ((segmentsRestants - FREADSIZE > 0)){
			segmentsRestants = segmentsRestants - FREADSIZE;
			nbSegInThisPartie = FREADSIZE;
		} else {
			nbSegInThisPartie = segmentsRestants;
		}
		
		int l;//kompteur
		for (l=0; l<nbSegInThisPartie; l++){ //Seg 1 est stocké dans contenuBuff[0] et sa taille dans contenuBuffSize[0]
			if ( (nbBytes = fread(contenuBuff[l],sizeof(char),TABSIZE-6,f))<0){	
				perror ("Erreur copie octets\n");
			}
			contenuBuffSize[l] = nbBytes;
		}
		int bonSegment = 0; // va contenir l'index du bon segment a envoyer 
		int ehOuiCestLaFin =0; // quand 1, alors on a recu le dernier ack
		int finDeLaPartie = 0; //quand on arrive au bout d'une partie	
		int tmp=0;
		int finRetransmission = 0; //savoir quand on arrete la retransmission apres timeout
		
		
		
		//envoi des segments du fichier

		
		
		
		
		if(LOGS){
			printf("nombre de segments a envoyer : %d\n",nbseg);
		}
		for(i=0;i<6;i++){
			tab[i]='\0';
		}
		
		//On s'arrete quand on a recu le dernier ack
		while(!ehOuiCestLaFin){
			
			if(seg<nbseg && partiesNonTraitees>0){//s'il y a encore des segments a envoyer
						
						if (LOGS){
							printf("INFO : parties non traitees = %d \n", partiesNonTraitees);
						}
						//envoi en burst de segments
						while( (seg < window+segaack) && (numsegrecu <= nbseg) &&  !finDeLaPartie ){
							if (LOGS){
								printf("\n---------- ENVOI D'UN  SEGMENT ---------- \n");
								printf("DEBUG : segment = %d - window = %d - segack - %d\n",seg, window, segaack);
							}
							
							//recuperation du bon index
							if ((seg)%FREADSIZE == 0 && seg != 0){
								if (LOGS){
									printf("INFO : arrivee en fin de partie, seg =%d \n", seg);
								}
								bonSegment = seg-1;
								finDeLaPartie = 1; //on est arrivé en bout de partie
							} else if (seg == 0) {
								if (LOGS){
									printf("INFO : premier segment de la partie, seg =%d \n", seg);
								}
								bonSegment = 0;
							} else
							{
								if (LOGS){
									printf("INFO : segment classique, seg =%d \n", seg);
								}
								bonSegment = (seg%FREADSIZE)-1;
							}
							
							for(i=0;i<6;i++){
								tab[i]='\0';
							}
							
							sprintf(tab, "%d", seg);
							for(i=0;i<contenuBuffSize[bonSegment];i++){
								tab [i+6]=contenuBuff[bonSegment][i];
							}
							
							if (LOGS){
								printf("DEBUG : nb bytes a envoyer =%d\n", contenuBuffSize[bonSegment]);
							}
							
							if(sendto(descenv2, tab, contenuBuffSize[bonSegment]+6, 0, (struct sockaddr *)&adresseenv2, taille)==-1){
								perror("sendto file error\n");
							}
							gettimeofday(&start[seg%100], NULL);//obtenir temps systeme pour rtt
							if (LOGS){
								printf("INFO : segment n° %d sent sur %d\n",seg, nbseg);
							}
							if(seg==nbseg){break;}
							seg++;
							segaenv--;
						}
						
			} 
			
			
			attente = getAttente((int)(rttmoy*1000.0));
   			rv = poll(fds, nfds, attente);

			if (rv < 0)
    		{
    			perror("poll() failed, pitié que ca n'arrive jamais \n");
    			break;
   			}
   			
   			
			if (rv == 0)//timeout : on renvoie le burst précédent
    		{
				
				sstresh=flightSize/2;
				window=3;
				tmp = numsegrecu+1;
				segaenv=window;
				
				if (LOGS){
					printf("\n---------- TIMEOUT DETECTE ----------\n");
					printf("ERREUR : segment perdu car poll timeout : rtt : %d ms \n",attente);
					printf("INFO : envoi en burst de ce qui a ete perdu, tmp debut = %d \n", tmp);
				}
				
				
				finRetransmission = 0;
				while ( (tmp<=seg) && !finRetransmission ){
					
					if (LOGS){
						printf("DEBUG : tmp = %d - seg = %d \n", tmp, seg);
					}
				
					if ((tmp)%FREADSIZE == 0 && tmp !=0){
						if (LOGS){
							printf("INFO : arrivee en fin de retaransmisison \n");
						}
						bonSegment = tmp-1;
						finRetransmission = 1;
					} else if (tmp == 0) {
						bonSegment = 1;
					} 
					else {
						bonSegment = (tmp%FREADSIZE)-1;
					}
					
					for(i=0;i<6;i++){
						tab[i]='\0';
					}
					
					sprintf(tab, "%d", (numsegrecu+1));
					for(i=0;i<contenuBuffSize[bonSegment];i++){
						tab [i+6]=contenuBuff[bonSegment][i];
					}
					if(sendto(descenv2, tab, contenuBuffSize[bonSegment]+6, 0, (struct sockaddr *)&adresseenv2, taille)==-1){
						perror("sendto file error\n");
					}
					tmp++;
				}
				
							
				
				isItEOF = 0;
							
    		}
    		
    		
    		
    		
    		else if (rv > 0) {
   		    	 if (fds[0].revents & POLLIN) {
					if (LOGS){
						printf("\n---------- ARRIVEE D'UN ACK ----------\n");
					}
   	    		    if( recvfrom(descenv2, bufferrec, sizeof(bufferrec), 0,  (struct sockaddr *) &adresseenv2, &taille) <= 0 )
						{
							perror( "recvfrom() error \n" );
							return -1;
						}
					numsegrecu=extractack(bufferrec);
					if (LOGS){
						printf("DEBUG : ACK recu = %d - ACK attendu = %d \n", numsegrecu, segaack);
					}
					//calcul rtt

						gettimeofday(&end[numsegrecu%100], NULL);
						if(numsegrecu==nbseg){
							break;

						}
				
						
						t1 = start[numsegrecu%100].tv_sec+(start[numsegrecu%100].tv_usec/1000000.0);
						t2 = end[numsegrecu%100].tv_sec+(end[numsegrecu%100].tv_usec/1000000.0);
						rtt = (t2-t1);
						rttmoy = (rttmoy*(nbelemrttmoy-1) + rtt)/nbelemrttmoy; // running average sur nbelemrttmoy
						

						if(segaack<=numsegrecu){ // si on recoit un ACK correct
							
							//maj du flightSize
							flightSize = updateFlightSize(seg, numsegrecu);
							//calcul nombre segments acquittés
							nbAckSent = (numsegrecu-segaack)+1;
							
							if (LOGS){
								printf("INFO : reception ACK correct(cad >= a ACK attendu)\n");
								printf("INFO : ssthresh = %d - Flight Size = %d (seg = %d - ack recu = %d) - nb segs a acquitter = %d\n", sstresh,seg,numsegrecu, flightSize, nbAckSent);
							}
		
		
		
							if (numsegrecu%FREADSIZE == 0 && numsegrecu !=0){//si on recoit l'ack du dernier segment du tableau
								
								if (LOGS){
									printf("INFO, reception dernier segment d'une partie\n");
								}
								partiesNonTraitees --;
								
								if (partiesNonTraitees == 0){
									if (LOGS){
										printf("INFO, reception du dernier ack de la transmission : %d\n", numsegrecu);
									}
									
									ehOuiCestLaFin =1;
								} else { // on crée le nouveau tableau de segments
									if (LOGS){
										printf("INFO : fin partie, mais il reste des parties encore : %d\n", numsegrecu);
									}
									finDeLaPartie = 0;
									if ((segmentsRestants - FREADSIZE > 0)){
										segmentsRestants = segmentsRestants - FREADSIZE;
										nbSegInThisPartie = FREADSIZE;
									} else {
										nbSegInThisPartie = segmentsRestants;
									}
									if (LOGS){
										printf("DEBUG : nbseg a envoyer ds la partie =  %d\n", nbSegInThisPartie);
									}
									
									for (l=0; l<nbSegInThisPartie; l++){ //Seg 1 est stocké dans contenuBuff[0] et sa taille dans contenuBuffSize[0]
										if ( (nbBytes = fread(contenuBuff[l],sizeof(char),TABSIZE-6,f))<0){	
											perror ("Erreur copie octets\n");
										}
										contenuBuffSize[l] = nbBytes;
									}
								}
								
								
								
							}
							
							for (cpt=0;cpt < nbAckSent; cpt++){
								
								//congestion avoidance : la window "augmente" de 1/window a chaque ACK
								if(window > sstresh){
									if (windowAcc < window){
										if (LOGS){
											printf("INFO : C.A, window augmente pas et reste a %d\n", window);
										}
										windowAcc ++;
									}
									else {
										if (LOGS){
											printf("INFO : C.A, window passe de %d a %d\n", window, window+1);
										}
										window = window+1;
										windowAcc = 0;
										segaenv=window;
									}
								//slow start
								}else{
									if (LOGS){
										printf("INFO : slow start, window passe de %d a %d\n", window, window+1);
									}
									window++;
									segaenv=window;
								}
								
								segaack++;	
							}
							duplicateACK = 0;
							lastReceivedACK = numsegrecu;
							
							
							
							
						}
						else if (lastReceivedACK == numsegrecu){ // si on recoit a nouveau l'ACK precedent, perte potentielle
							
							if(duplicateACK<3){ 
								if (LOGS){
									printf("INFO : reception de cet ACK pour la %d fois \n", duplicateACK);
								}
								duplicateACK++;
								
							}else{ //seg perdu : on le renvoie 3 fois !
							
								
								sstresh=flightSize/2;
								window=3;
								if (LOGS){
									printf("INFO : segment perdu car meme ACK 3 fois \n");
									printf("INFO : nouveaux params : ssthresh = %d - window = %d - seg à renvoyer = %d \n", sstresh, window, numsegrecu+1);
								}
								
								
								//recuperation du bon index
								if ((numsegrecu+1)%FREADSIZE == 0){
									bonSegment = FREADSIZE;
								} else {
									bonSegment = ((numsegrecu+1)%FREADSIZE)-1;
								}
								if (LOGS){
									printf("INFO : index seg a renvoyer = %d, seg = %d \n", bonSegment, numsegrecu+1);
								}
								
								for(i=0;i<6;i++){
									tab[i]='\0';
								}
								sprintf(tab, "%d", (numsegrecu+1));
								
								for(i=0;i<contenuBuffSize[bonSegment];i++){
									tab [i+6]=contenuBuff[bonSegment][i];
								}
								//envoi 3 fois du seg
								for (j=0; j<3;j++){
									if(sendto(descenv2, tab, contenuBuffSize[bonSegment]+6, 0, (struct sockaddr *)&adresseenv2, taille)==-1){
										perror("sendto file error\n");
									}
								}

								duplicateACK=0;

								
							}
						}
						else{//si ACK recu < dernier ack, on l'ignore car c'est un ack arrivé en retard
							if (LOGS){
								printf("INFO : ACK non traité car arrivé en retard ! \n");
							}
						}
    	    	}
    		}	
		}
		
		fclose(f);
		strcpy(tab, "FIN");
		if(sendto(descenv2, tab, sizeof(tab), 0, (struct sockaddr *)&adresseenv2, taille)==-1){
			perror("sendto FIN error\n");
		}
		printf("INFO : nb segs totaux = %d - dernier ack recu = %d - seg courant = %d\n",nbseg, numsegrecu, seg);
		return 0;
	}

int extractack(char * t){
	int i,j=0;
	char tack[4];
	char tnumseg[6];
	for (i=0;i<3;i++){
		tack[i]=t[i];
	}
	for (i=3;i<9;i++){
		tnumseg[j]=t[i];
		j++;
	}
	int numseg=atoi(tnumseg);
	if(strncmp("ACK",tack,3)==0){
		return (numseg);
	}else{
		printf("ERROR : ACK : |%s|\n",tack);
		return (-1);
	}
}

int getTaille(FILE* fp){
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    rewind(fp);
    return size;
    
    
}

//return the new flightSize
int updateFlightSize(int seg, int numsegrecu){
	return (seg - numsegrecu);
}

//python3 launch.py serveur 192.168.5.298 8080 0 sample4_l.jpg 1         //test scenario 1
//python3 launch.py serveur 192.168.5.298 8080 0 sample4_l.jpg 2         //test scenario 2
//python3 launch.py serveur 192.168.5.298 8080 0 sample4_l.jpg 3         //test scenario 3  a corriger 
//./client1 192.168.5.298 8080 sample4_l.jpg   
//./client1 192.168.0.50 8080 sample4_l.jpg   
//./serveur 8080 1
