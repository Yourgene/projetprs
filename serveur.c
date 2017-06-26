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
#include <math.h>
#include <pthread.h>
#define BUFFSIZE 1494
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

	char addrserv[15];//="192.168.5.298"; //192.168.0.50    ou    192.168.5.298
	sprintf(addrserv,"%s",argv[3]);
	printf("INFO : serveur lance sur l' adresse %s\n",addrserv);
	while (1) {
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
						/*read = recvfrom(desccli, buffer, sizeof(buffer), 0,  (struct sockaddr *) &client, &taillecli);
						if( read <= 0 )
						{
							perror( "recvfrom() error \n" );
							return -1;
						}
						printf("INFO : debut envoi fichier %s\n",buffer);*/
					}else{return -1;}
					/** VARIABLES **/

    char bufferRCV[RCVSIZE];
    char bufferSEND[BUFFSIZE];
    int reutilisation = 1;
    FILE *file;
    int numACK=0;
    char tabACK[6];
    int sizeBuffer;
    int cwnd=100;
    int LastACK=1;
    int ack = 0;
    int ENDack=0;
    char bufferLecture[BUFFSIZE-6];
    //int port=port;


    /** STRUCTURES **/

    //struct sockaddr_in  client;
    socklen_t alenPrive = sizeof(client);
    fd_set Ldesc;
    struct timeval tv;
    tv.tv_sec=1;

    /** SOCKET data **/

    memset(bufferRCV, 0, RCVSIZE);
    recvfrom(desccli, bufferRCV, RCVSIZE, 0, (struct sockaddr*)&client, &alenPrive);
    numACK =1;
    int onContinue = 1;
    int ssthresh=0;
    int CongestionAvoidance=0;
    int j;
    int flightSize=0;
    char ACKrecu[6] = "AAAAAA";
    struct timeval tpsSend;
    struct timeval tpsRcv;
    int rttcalcul=0;
    tv.tv_sec=0;
    tv.tv_usec =5000;
    int resend=1;
    printf("Fichier recherché : %s\n", bufferRCV);
    file = fopen(bufferRCV,"r");
    if(file==NULL){
       printf("Erreur d'ouverture du fichier, Veuillez relancer le serveur.\n");
        exit(1);
     }

     //Lecture du fichier
     //On continue tant qu on est pas a la fin ou que y a qq chose a envoyer
     while(onContinue == 1 || resend==1 || LastACK != ENDack) {
		if(rttcalcul==1){
			tv.tv_usec = ((tpsRcv.tv_sec*1000000+tpsRcv.tv_usec) - (tpsSend.tv_sec*1000000+tpsSend.tv_usec) )+1000;
			rttcalcul=0;
			printf("mon RTT: %ld \n", tv.tv_usec);
		}else{
			tv.tv_usec =5000;
		}
         resend=0;//on met le renvoi a 0
         numACK = LastACK;
         fseek(file, (numACK-1)*(BUFFSIZE-6), SEEK_SET);//On se place dans le fichier
          //printf("On envoie a partir du dernier ACK recu : %d\n", LastACK);

          for(flightSize=0; flightSize<cwnd;flightSize++) {
              //memset(tabACK, 0, 6);
              memset(bufferSEND, '0', BUFFSIZE);
              memset(bufferLecture, 0, BUFFSIZE-6);
              sprintf(tabACK, "%06d", numACK);
              memcpy(bufferSEND, tabACK, strlen(tabACK));
              //printf("ACK envoyé: ");
              /*for(j=0; j<6; j++) {
                  printf("%c",bufferSEND[j]);
                 }
                printf("\n");*/
               sizeBuffer=fread(bufferLecture, 1, BUFFSIZE-6, file);//On lit le fichier
               memcpy(&bufferSEND[6], bufferLecture, BUFFSIZE-6);//On met le buffer d'envoi a jour
                //On envoi
               
               sendto(desccli, bufferSEND, sizeBuffer+6, 0, (struct sockaddr*)&client, alenPrive);
				

               numACK++;
               
               if(feof(file)){
                  onContinue=0;
                  printf("dernier ack : %d \n\n", ENDack);
                  ENDack = numACK-1;
                  flightSize=cwnd;
                  break;
                }
            }
            gettimeofday(&tpsSend, NULL);
            
            for(flightSize=0; flightSize<cwnd;flightSize++) {
               //memset(tabACK, 0, 6);
               memset(bufferSEND, '0', BUFFSIZE);
               //memset(bufferLecture, 0, BUFFSIZE-6);
               //sprintf(tabACK, "%d", numACK);
               FD_ZERO(&Ldesc);
               FD_SET(desccli,&Ldesc);
               //printf("On va faire le select\n");
               if(tv.tv_usec<=0){
				   tv.tv_usec=5000;
			   }
               select(desccli+1,&Ldesc,NULL,NULL, &tv);
               if(FD_ISSET(desccli,&Ldesc)==1) { //On recoit
               //On lit ce qu'on a reçu
               recvfrom(desccli, bufferRCV, RCVSIZE, 0, (struct sockaddr*)&client, &alenPrive);
               if(flightSize==0){
				  gettimeofday(&tpsRcv, NULL);
				  if(rttcalcul==0){
					  rttcalcul=1;
				  }
				}
               //On regarde l'ACK reçu
               for(j = 0; j<6; j++) {
                   ACKrecu[j] = bufferRCV[j+3];
               }
               ack = atoi(ACKrecu);
				if(ack == ENDack){
					onContinue=0;
					resend=0;
					LastACK = ack;
					printf("dernier ack reçu %d \n",ack);
					break;
				}
                if(ack>LastACK) {
                   LastACK = ack;
                   if(CongestionAvoidance==1){//slow start
										cwnd+=2.5;
									}else{
										cwnd+=7;
									}
                                }
                            } else { //On envoie
								if(ssthresh==0){
									if(cwnd/2>=cwnd-5){
										ssthresh=cwnd/2;
									}else{
										ssthresh=cwnd-5;
									}
									CongestionAvoidance=1;//fin du slow start, debut de congestion avoidance
								}
								cwnd=ssthresh;
                                break;
                            }
                        }

                    }
                    int w;
                    for(w=0; w<50; w++){ //On envoie le segment fin 3 fois afin de s'assurer de la reception'
						if(w>4){
							sleep(0.1);
						}
						sendto(desccli, "FIN", sizeof("FIN"), 0, (struct sockaddr*)&client, alenPrive);
					}
				}else{
					close(descserv);//permet de ne pas court circuiter la reception de ack dans le fils
					sleep(1);//NE PAS SUPPRIMER
				/* Si on est dans le père */

				}
				
			}else{
				printf("ERROR : message SYN invalide : %s\n", buffer);
			}
			sleep(1);
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