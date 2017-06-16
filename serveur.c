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


int envoifile(char* nomf, int descenv2, struct sockaddr_in adresseenv2);
int extractack(char * t);
int getTaille(FILE* fp);


//return the new flightSize
int updateFlightSize(int seg, int numsegrecu){
	return (seg-numsegrecu);
}

struct timeval end[1000], start[1000];

double t1,t2;

int main (int argc, char *argv[]) {
	if(argc!=2){
		printf("ERROR : necessite un argument : appel sous la forme './serveur numport'\n");
		exit(-1);
	}
	printf("INFO : serveur lance sur l' adresse 192.168.5.298\n");
	struct sockaddr_in serveur;
	int port = atoi(argv[1]);
	int portserv=port;
	int valid= 1;
	char buffer[RCVSIZE];
	char str[4];

	char addrserv[]="192.168.0.50";
	
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
						//printf("ACK received : %s \n",buffer);
						if( read <= 0 )
						{
							perror( "recvfrom() error \n" );
							return -1;
						}
						printf("INFO : debut envoi fichier %s\n",buffer);
						envoifile(buffer,desccli, client);
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
		//taille segment 1005
		socklen_t taille = sizeof(adresseenv2);
		int i,cpt,nbelemrttmoy=5;
		char tab[1406];
		int nbseg, seg=1;
		int taillef=0;
		double rtt=0, rttmoy=(0.05); //valeur de rttmoy pris comme valeur initiale. ne compte que pour les premieres trames
		int lastseg=0;
		int sstresh = 9999999;
		int duplicateACK=0;
		int window = 1;
		int segaack=1;
		//variable ack
		int numsegrecu;
		char bufferrec[RCVSIZE];
		int flightSize =0;
		int windowAcc = 0; //incrémenteur utilisé pour le congestion avoidance
		int nbAckSent=0;
		int lastReceivedACK=0;
		//ouverture fichier
		FILE* f = NULL;
		f = fopen(nomf,"r");
		if (f != NULL){
        	// On peut lire et écrire dans le fichier
    	}else{
        	printf("Impossible d'ouvrir le fichier %s\n",nomf);
    	}
		//init pour poll()
		int rv;
		struct pollfd ufds[1];
		ufds[0].fd = descenv2;
		ufds[0].events = POLLIN;
		taillef = getTaille(f);
		//printf("taille du fichier : %d\n",taillef);

		sprintf(tab, "%d", taillef);
		//calcul du nombre de segments requis
		if(taillef%1400!=0){
			nbseg = (taillef/1400);
			lastseg = taillef%1400;
			//printf("lastseg : %d\n",lastseg);
		}else{
			nbseg = (taillef/1400);
		}
		i=0;
		//envoi des segments du fichier
		do{
			//acquittement des segments reçus
			// wait for events on the sockets, 0ms timeout
   			rv = poll(ufds, 1, rttmoy);
    		if (rv > 0) {
   		    	 if (ufds[0].revents & POLLIN) {
					//printf("receiving %d %d %d %d\n", descenv2, ntohl(adresseenv2.sin_addr.s_addr), ntohs(adresseenv2.sin_port), taille);
   	    		    if( recvfrom(descenv2, bufferrec, sizeof(bufferrec), 0,  (struct sockaddr *) &adresseenv2, &taille) <= 0 )
						{
							perror( "recvfrom() error \n" );
							return -1;
						}
					numsegrecu=extractack(bufferrec);
					printf("ACK recu : %d\n",numsegrecu);
					//calcul rtt

						gettimeofday(&end[numsegrecu%100], NULL);
						//printf("gettime OK \n");
						//printf("received2 %d, modulo : %d\n", numsegrecu, numsegrecu%100);
						t1 = start[numsegrecu%100].tv_sec+(start[numsegrecu%100].tv_usec/1000000.0);
						t2 = end[numsegrecu%100].tv_sec+(end[numsegrecu%100].tv_usec/1000000.0);
						rtt = (t2-t1);
						rttmoy = (rttmoy*(nbelemrttmoy-1) + rtt)/nbelemrttmoy; // running average sur nbelemrttmoy
						//printf("DEBUG : RTT : %f %d\n", rttmoy, numsegrecu);
						


						//printf("DEBUG : RTT : %f\n", rttmoy);
						if(segaack<=numsegrecu){ // si on recoit un AC correct
							
							flightSize = updateFlightSize(seg, numsegrecu);
							//maj du flightSize || a verifier mais ta valeur de seg ici n' a aucun sens
							// flightsize = nb segments envoyés mais pas acquittés, cad seg - ackrecu. 
		
						
							
							//calcul nombre segments acquittés
							nbAckSent = (numsegrecu-segaack)+1;
							
							for (cpt=0;cpt < nbAckSent; cpt++){
								
								//congestion avoidance : la window "augmente" de 1/window a chaque ACK
								if(window > sstresh){
									if (windowAcc < window){
										printf("INFO : C.A, window augmente pas et reste a %d\n", window);
										windowAcc ++;
									}
									else {
										printf("INFO : C.A, window passe de %d a %d\n", window, window+1);
										window = window+1;
										windowAcc = 0;
									}
								//slow start
								}else{
									printf("INFO : slow start, window passe de %d a %d\n", window, window+1);
									window++; 
								}
								
								segaack++;	
							}
							duplicateACK = 0;
							lastReceivedACK = numsegrecu;
							
						}
						else if (lastReceivedACK == numsegrecu){ // si on recoit a nouveau l'ACK precedent, perte potentielle
							
							if(duplicateACK<3){ 
								printf("INFO : reception du meme ACK %d a nouveau \n", numsegrecu);
								duplicateACK++;
							}else{
								sstresh=flightSize/2;
								window=1;
								seg = numsegrecu+1;
								printf("INFO : paquet %d perdu, reprise a fenetre = 1\n", numsegrecu);
								duplicateACK=0;
								//NOTE : ajouter une nouvelle variable pour contenir le segment a renvoyer
							}
						}
						else{//si ACK recu < dernier ack, on l'ignore car c'est un ack arrivé en retard
							printf("INFO : ACK %d recu mais non traité car en retard, nous sommes a l'ACK %d \n", numsegrecu, segaack);
							/* A VIRER
							 * if((duplicateACK<3)&&(numsegrecu==(segaack-(1+duplicateACK)))){ // a quoi l'expression apres le '&&' maxime ?
								printf("INFO : reception du meme ACK %d a nouveau \n", numsegrecu);
								duplicateACK++;
							}else{
								sstresh=flightSize/2;
								window=1;
								seg = numsegrecu+1;
								printf("INFO : paquet %d perdu, reprise a fenetre = 1\n", numsegrecu);
								duplicateACK=0;
								//NOTE : ajouter une nouvelle variable pour contenir le segment a renvoyer
							}*/
						}
    	    	}
    		}else{
					

				if(seg<nbseg){//s'il y a encore des segments a envoyer
						
						while(seg<=(window+segaack)){//utilisation de la window de la window
						printf("DEBUG : seg = %d - window = %d - segack - %d\n",seg, window, segaack);
							for(i=0;i<6;i++){
								tab[i]='\0';
							}
							sprintf(tab, "%d", seg);
							for(i=6;i<1406;i++){
								tab[i]=fgetc(f);
							}
							
							//printf("tab : %s\n",tab);
							if(sendto(descenv2, &tab, sizeof(tab), 0, (struct sockaddr *)&adresseenv2, taille)==-1){
								perror("sendto file error\n");
							}
							gettimeofday(&start[seg%100], NULL);//obtenir temps systeme pour rtt
							printf("segment n° %d sent sur %d\n",seg, nbseg);
							seg++;
							
						}
						
				}else{//dernier segment

					if(lastseg!=0){
						i=0;
						for(i=0;i<6;i++){
							tab[i]='\0';
						}
						sprintf(tab, "%d", seg);
						do{	
							tab[i]=fgetc(f); 
							i++;
						}
						while(i<lastseg+6);
						tab[i]='\0';
					//	printf("tab : %s\n",tab);
						if(sendto(descenv2, tab, sizeof(tab), 0, (struct sockaddr *)&adresseenv2, taille)==-1){
							perror("sendto file error\n");
						}	
					}
				}
			}
		}
		while(numsegrecu!=nbseg);
		printf("numsegrecu : %d\n",numsegrecu);
		fclose(f);
		strcpy(tab, "FIN");
		if(sendto(descenv2, tab, sizeof(tab), 0, (struct sockaddr *)&adresseenv2, taille)==-1){
			perror("sendto FIN error\n");
		}
		
			//printf("%d %d %d %d %d\n",descenv2,sizeof(tab),ntohs(adresseenv2.sin_port),ntohl(adresseenv2.sin_addr.s_addr),taille);
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

//python3 launch.py serveur 192.168.5.298 8080 sample4_l.jpg 1         //test scenario 1
//python3 launch.py serveur 192.168.5.298 8080 sample4_l.jpg 2         //test scenario 2
//python3 launch.py serveur 192.168.5.298 8080 sample4_l.jpg 3         //test scenario 3  a corriger 
//./client1 192.168.5.298 8080 sample4_l.jpg
//./serveur 8080
