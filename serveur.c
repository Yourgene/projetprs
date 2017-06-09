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

#define RCVSIZE 1024


int envoifile(char* nomf, int descenv2, struct sockaddr_in adresseenv2);

int main (int argc, char *argv[]) {
	struct sockaddr_in serveur;
	int port = 8080;
	int portserv=port;
	int valid= 1;
	char buffer[RCVSIZE];
	char str[4];
	
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
			int read = recvfrom(descserv, &buffer, sizeof(buffer), 0,  (struct sockaddr *) &serveur, &taille);
     
			if( read <= 0 )
			{
				perror( "recvfrom() error \n" );
				return -1;
			}
			//printf("INFO : SYN : %s\n",buffer);
			if(strcmp("SYN",buffer)==0){
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
					inet_aton("127.0.0.1",&client.sin_addr);
					if (bind(desccli, (struct sockaddr*) &client, sizeof(client)) == -1) {
						perror("blind descserv2 fail\n");
						close(desccli);
						exit(-1);
					}
					FD_SET(desccli, &rfds);
					int tb = 12;
					char buffersyn[tb];
					strcpy(buffersyn, "SYN-ACK|");
					sprintf(str, "%d", port);
					strcat(buffersyn, str);
					//printf("buffersynack : %s \n", buffersyn);
					sendto(descserv, &buffersyn, sizeof(buffersyn), 0, (struct sockaddr *)&serveur, taille);
					printf("INFO : SYN-ACK sent \n");
					read = recvfrom(desccli, &buffer, sizeof(buffer), 0,  (struct sockaddr *) &client, &taillecli);
     
					if( read <= 0 )
					{
						perror( "recvfrom() error \n" );
						return -1;
					}
					printf("ACK : %s\n",buffer);
			
			
					if(strcmp("ACK",buffer)==0){
						read = recvfrom(desccli, &buffer, sizeof(buffer), 0,  (struct sockaddr *) &client, &taillecli);
						//printf("ACK received : %s \n",buffer);
						if( read <= 0 )
						{
							perror( "recvfrom() error \n" );
							return -1;
						}
						envoifile(buffer,desccli, client);
						}
						close(desccli);
				}else{

				/* Si on est dans le père */

				}
				
			}else{
				printf("INFO : message SYN invalide \n");
			}
	}


return 0;
}



int envoifile(char* nomf, int descenv2, struct sockaddr_in adresseenv2){
		//taille segment 1005
		socklen_t taille = sizeof(adresseenv2);
		int i;
		char tab[1005];
		char c;
		int nbseg, seg=0;
		char bufferack[RCVSIZE];
		int taillef=0;
		int lastseg=0;
		//variable ack
		char ackseg[5];
		char bufferrec[RCVSIZE];
		//ouverture fichier
		FILE* f = NULL;
		f = fopen(nomf,"r");
		if (f != NULL){
        	// On peut lire et écrire dans le fichier
    	}else{
        	printf("Impossible d'ouvrir le fichier test.pdf");
    	}
		//envoi d'un message prevenant de l'envoi d'un fichier
		/*char txt[]="file";
		//printf("envoi taille fichier\n");
		if(sendto(descenv2, &txt, sizeof(txt), 0, (struct sockaddr *)&adresseenv2, taille)==-1){
			perror("send preventing msg error\n");
		}*/
		//envoi taille fichier
		while(1)
	  	{
   	   		taillef++;
			c=fgetc(f);
    		if( feof(f) )
   			{ 
    			break ;
    		}
   		}
		rewind(f);

		//printf("taille du fichier : %d\n",taillef);
		sprintf(tab, "%d", taillef);
	//	printf("tab taille : %s\n",tab);
		if(sendto(descenv2, &tab, sizeof(tab), 0, (struct sockaddr *)&adresseenv2, taille)==-1){
			perror("sendto file error\n");
		}
		//calcul du nombre de segments requis
		if(taillef%1000!=0){
			nbseg = (taillef/1000);
			lastseg = taillef%1000;
			//printf("lastseg : %d\n",lastseg);
		}else{
			nbseg = (taillef/1000) + 1;
		}
		i=0;
		//envoi des segments du fichier
		do{
			for(i=0;i<5;i++){
				tab[i]='\0';
			}
			sprintf(tab, "%d", seg);
			for(i=5;i<1005;i++){
				tab[i]=fgetc(f);
			}
			
			//printf("tab : %s\n",tab);
			if(sendto(descenv2, &tab, sizeof(tab), 0, (struct sockaddr *)&adresseenv2, taille)==-1){
				perror("sendto file error\n");
			}
			//	printf("segment n° %d sent\n",seg);
			
			//acquittement des segments reçus
			if( recvfrom(descenv2, &bufferrec, sizeof(bufferrec), 0,  (struct sockaddr *) &adresseenv2, &taille) <= 0 )
			{
				perror( "recvfrom() error \n" );
				return -1;
			}
			strcpy(bufferack, "ACK_");
			sprintf(ackseg, "%d", seg);
			strcat(bufferack, ackseg);
			if(strcmp(bufferack,bufferrec)==0){
				printf("Segment %d ACK\n",seg);
			}else{
				printf("Segment %d NACK\n",seg);
				printf("bufferack : %s\n buferrec : %s\n",bufferack,bufferrec);

			}
			//fin ack
			seg++;
		}
		while(seg<nbseg);
		if(lastseg!=0){
			i=0;
			for(i=0;i<5;i++){
				tab[i]='\0';
			}
			sprintf(tab, "%d", seg);
			do{	
				tab[i]=fgetc(f);
				i++;
			}
			while(i<lastseg+4);
			tab[i]='\0';
		//	printf("tab : %s\n",tab);
			if(sendto(descenv2, &tab, sizeof(tab), 0, (struct sockaddr *)&adresseenv2, taille)==-1){
				perror("sendto file error\n");
			}
			//printf("segment lastseg n° %d sent\n",seg);
			//acquittement des segments reçus
			if( recvfrom(descenv2, &bufferrec, sizeof(bufferrec), 0,  (struct sockaddr *) &adresseenv2, &taille) <= 0 )
			{
				perror( "recvfrom() error \n" );
				return -1;
			}
			strcpy(bufferack, "ACK_");
			sprintf(ackseg, "%d", seg);
			strcat(bufferack, ackseg);
			if(strcmp(bufferack,bufferrec)==0){
				printf("Segment (lastseg) %d ACK\n",seg);
			}else{
				printf("Segment (lastseg) %d NACK\n",seg);
				printf("bufferack : %s\n buferrec : %s\n",bufferack,bufferrec);

			}
			//fin ack
		}
		fclose(f);
		printf("INFO : fichier %s envoye\n", nomf);
			//printf("%d %d %d %d %d\n",descenv2,sizeof(tab),ntohs(adresseenv2.sin_port),ntohl(adresseenv2.sin_addr.s_addr),taille);
		return 0;
	}
