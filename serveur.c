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
#define TEMP 350000 //=1400*250




int envoifile(char* nomf, int descenv2, struct sockaddr_in adresseenv2);
int extractack(char * t);
int getTaille(FILE* fp);
int updateFlightSize(int seg, int numsegrecu);
int getAttente(int rtt);

struct timeval end[1000], start[1000];
char renvoitab[250*1406];
char temptab[TEMP];
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

	char addrserv[]="192.168.5.298"; //192.168.0.50    ou    192.168.5.298
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
		//char octets[TABSIZE-6];
		int nbseg, seg=1;
		int taillef=0;
		double rtt=0, rttmoy=(0.005); //valeur de rttmoy pris comme valeur initiale : 50ms. ne compte que pour les premieres trames
		int sstresh = 65535;
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
		int j,k,l; //utilisés pour le tableau de renvoi
		int segaenv=1;
		int senv;
		int attente;
		int freadBytes;
		int segrenvoye=0;
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
		//envoi des segments du fichier
		//do{
		if(LOGS){
			printf("nombre de segments a envoyer : %d\n",nbseg);
		}
		for(i=0;i<6;i++){
			tab[i]='\0';
		}
		int nbByteslast;
		int cestledernier=0;
		while(numsegrecu != nbseg){
			//printf("debut de boucle\n");
			if(seg<nbseg+1){//s'il y a encore des segments a envoyer
				while((/*seg<=window+segaack*/segaenv>0)&&(numsegrecu <= nbseg+1)){//utilisation de la window 
					if (LOGS){
						printf("\n---------- ENVOI D'UNE SERIE SEGMENTS ---------- \n");
						printf("DEBUG : segment = %d - window = %d - segack - %d segaenv = %d\n",seg, window, segaack, segaenv);
					}
					if((seg-1)%250==0){
						if ( (freadBytes = fread(temptab,sizeof(char),TEMP,f))<0){	
							perror ("Erreur copie octets\n");
						}
					}

					if(freadBytes!=TEMP){
						if(seg%250==nbseg%250){
							nbByteslast=freadBytes%1400;
							cestledernier=1;
						}else{
							nbBytes=1400;
						}
					}else{
						nbBytes=1400;
					}

					k=0;
					sprintf(tab, "%d", seg);
					for(i=((seg-1)%250)*1400;i<((((seg-1)%250)*1400)+1400);i++){
						tab [k+6]=temptab[i];
						k++;
					}
					
					if (LOGS){
						printf("DEBUG : nb bytes lus =%d\n", nbBytes);
						
					}
					k=0;
					for(j=(((seg-1)%250)*1406);j<(((seg-1)%250)*1406)+1406;j++){
						renvoitab[j]=tab[k];
						k++;
					}
					if(cestledernier==0){
						if(sendto(descenv2, tab, nbBytes+6, 0, (struct sockaddr *)&adresseenv2, taille)==-1){
							perror("sendto file error\n");
						}
					}else{
						if(sendto(descenv2, tab, nbByteslast+6, 0, (struct sockaddr *)&adresseenv2, taille)==-1){
							perror("sendto file error\n");
						}
					}
					gettimeofday(&start[seg%100], NULL);//obtenir temps systeme pour rtt
					if (LOGS){
						printf("INFO : segment n° %d sent sur %d\n",seg, nbseg);
					}
					if(seg==nbseg){break;}
					seg++;
					segaenv--;
				}	
			}else{break;}
			
			//acquittement des segments reçus
			//printf("debut getAttente()\n");
			attente = getAttente((int)(rttmoy*1000.0));
			//sleep(attente);
			//printf("rv = poll(fds, %d, %d)\n",nfds, attente);
   			rv = poll(fds, nfds, attente);
			//printf("debut du poll()\n");
			if (rv < 0)
    		{
				//printf("fail\n");
    			perror("poll() failed");
    			break;
   			}
			if (rv == 0)
    		{
				//printf("timeout\n");
				flightSize = updateFlightSize(seg, numsegrecu);
				sstresh=flightSize/2;
				senv = (seg%250)-((numsegrecu+1)%250);
				/*if(fseek(f,(seg-1)*1400,SEEK_SET)){
					perror("fseek timeout failed\n");
				}*/
				//for(l=senv;l<seg%250;l++){
					//printf("renvoi du segment %d\n",l);
					//if(l>=nbseg){break;}
					if((senv%250==nbseg%250)&&(cestledernier==1)){
						k=0;
						for(j=(((senv)%250)*1406);j<(((senv)%250)*1406)+nbByteslast+6;j++){
							tab[k]=renvoitab[j];
							k++;
						}
						if(sendto(descenv2, tab, nbByteslast+6, 0, (struct sockaddr *)&adresseenv2, taille)==-1){
							perror("sendto file error\n");
						}
					}else{
						k=0;
						for(j=(((senv)%250)*1406);j<(((senv)%250)*1406)+1406;j++){
							tab[k]=renvoitab[j];
							k++;
						}
						if(sendto(descenv2, tab, nbBytes+6, 0, (struct sockaddr *)&adresseenv2, taille)==-1){
							perror("sendto file error\n");
						}
					}
					//gettimeofday(&start[l%100], NULL);//obtenir temps systeme pour rtt
					if (LOGS){
						printf("INFO : segment n° %d sent sur %d\n",numsegrecu+1, nbseg);
					}

					if (LOGS){
						printf("\n---------- TIMEOUT DETECTE ----------\n");
						printf("ERREUR : segment perdu car poll timeout : rtt : %d ms \n",attente);
						printf("INFO : nouveaux params : ssthresh = %d - window = %d - seg à envoyer = %d \n", sstresh, window, seg);
					}
				//}
				window=1;
				segaenv=window;
				//sleep(attente);
							
    		}
    		else if (rv > 0) {
				//printf("rv >0\n");
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
					
					if(numsegrecu==nbseg){break;}
					//printf("received2 %d, modulo : %d\n", numsegrecu, numsegrecu%100);
					t1 = start[(numsegrecu)%100].tv_sec+(start[(numsegrecu)%100].tv_usec/1000000.0);
					t2 = end[(numsegrecu)%100].tv_sec+(end[(numsegrecu)%100].tv_usec/1000000.0);
					rtt = (t2-t1);
					//printf("-----------------------------------------\nt1 : %f\n t2 : %f\n rtt : %f\n--------------------------------\n", t1, t2, rtt);
					rttmoy = (rttmoy*(nbelemrttmoy-1) + rtt)/nbelemrttmoy; // running average sur nbelemrttmoy	
					if(segaack<=numsegrecu){ // si on recoit un ACK correct		
						//calcul nombre segments acquittés
						nbAckSent = (numsegrecu-segaack)+1;		
						if (LOGS){
							printf("INFO : reception ACK correct(cad >= a ACK attendu)\n");
							printf("INFO : ssthresh = %d - Flight Size = %d (seg = %d - ack recu = %d) - nb segs a acquitter = %d\n", sstresh,seg,numsegrecu, flightSize, nbAckSent);
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
								}
								//slow start
							}else{
								if (LOGS){
									printf("INFO : slow start, window passe de %d a %d\n", window, window+1);
								}
								window++; 
							}	
							segaack++;		
							}
							duplicateACK = 0;
							lastReceivedACK = numsegrecu;
							if(numsegrecu>=seg-window){
								segaenv=window;
							}
					}else if (lastReceivedACK == numsegrecu){ // si on recoit a nouveau l'ACK precedent, perte potentielle		
						if((duplicateACK<3)&&(numsegrecu+1<=segrenvoye)){ 
							if (LOGS){
								printf("INFO : reception de cet ACK pour la %d fois \n", duplicateACK);
							}
							duplicateACK++;
						}else{ //seg perdu
							flightSize = updateFlightSize(seg, numsegrecu);
							sstresh=flightSize/2;
							window=1;
							segaenv=window;
							segrenvoye=numsegrecu+1;
							if((seg-numsegrecu)<250){
								if(LOGS){
									printf("INFO : paquet %d perdu, mais stocké dans le tableau\n", numsegrecu+1);
								}
								k=0;
								for(j=(((segrenvoye-1)%250)*1406);j<(((segrenvoye-1)%250)*1406)+1406;j++){ //-1 car index des tableaux commence a 0
									tab[k]=renvoitab[j];
									k++;
								}
								if(sendto(descenv2, tab, nbBytes+6, 0, (struct sockaddr *)&adresseenv2, taille)==-1){
									perror("sendto file error\n");
								}
								//gettimeofday(&start[seg%100], NULL);//obtenir temps systeme pour rtt
								//sleep(attente);
								if (LOGS){
									printf("INFO : segment n° %d sent sur %d\n",numsegrecu+1, nbseg);
								}
									
							}else{
								if(LOGS){
									printf("INFO : paquet %d perdu, et pas stocké dans le tableau\n", numsegrecu);
								}
								/*seg = numsegrecu+1;
								if(fseek(f,(seg-1)*1400,SEEK_SET)){
									perror("fseek duplicate failed\n");
								}*/
							}
							if(LOGS){
								printf("INFO : paquet %d perdu, reprise a fenetre = 1\n", numsegrecu);
							}
							duplicateACK=0;
							if (LOGS){
								printf("INFO : segment perdu car meme ACK 3 fois \n");
								printf("INFO : nouveaux params : ssthresh = %d - window = %d - seg à envoyer = %d \n", sstresh, window, seg);
							}
								
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

int getAttente(int rtt){
	//printf("rtt : %d\n",rtt);
	if (rtt <= 0) {
		return 1;
	}
	return rtt;
}

//python3 launch.py serveur 192.168.5.298 8080 0 sample4_l.jpg 1         //test scenario 1
//python3 launch.py serveur 192.168.5.298 8080 0 sample4_l.jpg 2         //test scenario 2
//python3 launch.py serveur 192.168.5.298 8080 0 sample4_l.jpg 3         //test scenario 3  a corriger 
//./client1 192.168.5.298 8080 sample4_l.jpg   ou 192.168.0.50
//./serveur 8080 1