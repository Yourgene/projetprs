#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define RCVSIZE 1024
char res2[7];
char portenv[5];
char * extract_syn(char * t, int taille);
int extract_port(char * t, int taille);
int receptionfile(int desccli, struct sockaddr_in client);

int main (int argc, char *argv[]) {
	
	struct sockaddr_in adresseenv2, adresseenv;
	/*if(argc!=3){
		printf("erreur : deux arguments : le port et l'@ip du serveur'\n");
		exit(-1);
	}
	printf("info : argv[0] %s\n",argv[0]);
	printf("info : port %d\n",atoi(argv[1]));
	printf("info : addr %s\n",argv[2]);*/
	int port= 8080;
	int valid= 1;
	char bufferutile[RCVSIZE];
	char bufferrec[RCVSIZE];
	//create socket
	int descenv= socket(AF_INET, SOCK_DGRAM, 0);

	// handle error
	if (descenv < 0) {
		perror("cannot create socket envoi\n");
		return -1;
	}
	
	socklen_t taille = sizeof(adresseenv);
	setsockopt(descenv, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
	

	adresseenv.sin_family= AF_INET;
	adresseenv.sin_port= htons(port);
	inet_aton("127.0.0.1", &adresseenv.sin_addr);
	
	char buffer[]="SYN";
	sendto(descenv, &buffer, sizeof(buffer), 0, (struct sockaddr *)&adresseenv, taille);
	
	 int read = recvfrom(descenv, &bufferrec, sizeof(bufferrec), 0,  (struct sockaddr *) &adresseenv, &taille);
     
			if( read <= 0 )
			{
				perror( "recvfrom() error \n" );
				return -1;
			}
			//printf("syn-ack : %s\n",bufferrec);
			close(descenv);
			int descenv2= socket(AF_INET, SOCK_DGRAM, 0);
			if (descenv2 < 0) {
				perror("cannot create socket envoi\n");
				return -1;
			}
			setsockopt(descenv2, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
			char *syn=extract_syn(bufferrec, sizeof(bufferrec));
			extract_port(bufferrec, sizeof(bufferrec));
			//printf("INFO : syn : |%s| port : %s \n",syn,portenv);
			//printf("\nportenv : %d",atoi(portenv));
			adresseenv2.sin_family= AF_INET;
			adresseenv2.sin_port= htons(atoi(portenv));
			inet_aton("127.0.0.1", &adresseenv2.sin_addr);
			taille = sizeof(adresseenv2);
			if(strcmp("SYN-ACK",syn)==0){
				char bufferack[]="ACK";
				if(sendto(descenv2, &bufferack, sizeof(bufferack), 0, (struct sockaddr *)&adresseenv2, taille) == -1){
					perror("error send ack\n");
				}
				//printf("INFO : ACK sent \n");
				strcpy(bufferutile, argv[1]);
				sendto(descenv2, &bufferutile, sizeof(bufferutile), 0, (struct sockaddr *)&adresseenv2, taille);
				//printf("INFO : message utile sent \n");
				receptionfile(descenv2, adresseenv2);
				strcpy(bufferutile, "FIN");
				sendto(descenv2, &bufferutile, sizeof(bufferutile), 0, (struct sockaddr *)&adresseenv2, taille);
				//printf("INFO : message utile sent \n");
				
			}else{
				printf("INFO : message SYN invalide \n");
			}
			close(descenv2);
	
	return 0;
}

int extract_port(char * t, int taille){
		int i, j=0, k;
		k=12;
		for (i=0;i<taille;i++){
			if( (i>=8) && (i<k) ) {
				portenv[j]=t[i];
				j++;
			}
		}
		portenv[4]='\0';
		return 0;
}
char * extract_syn(char * t, int taille){
		int i;
		
		for (i=0;i<7;i++){
			res2[i]=t[i];
		}
		res2[7]='\0';
		//printf("test : %s\n",res2);
		return res2;
	}
	
int receptionfile(int desccli, struct sockaddr_in client){
		FILE* f = NULL;
		socklen_t taillecli = sizeof(client);
		int i;
		int read;
		int nbseg, seg;
		char buffer2[1005];
		char buffseg[10];
		int lastseg=0;
		//variable ack
		char str[5];
		char buffersyn[RCVSIZE];

		f = fopen("res.jpg","w+");
		if (f != NULL){
			// On peut lire et écrire dans le fichier
		}else{
			printf("Impossible d'ouvrir le fichier test.pdf");
		}
		printf("reception taille fichier\n");
		read = recvfrom(desccli, &buffer2, sizeof(buffer2), 0,  (struct sockaddr *) &client, &taillecli);
		if( read <= 0 )
		{
			perror( "recvfrom() error \n" );
			return -1;
		}
		int taillef = atoi(buffer2);
		printf("taille du fichier : %d\n",taillef);
		//calcul du nombre de segments requis
	//	printf("taillefmodulo1000 : %d\n",taillef%1000);
		if(taillef%1000!=0){
			nbseg = (taillef/1000);
			lastseg = taillef%1000;
			printf("lastseg : %d\n",lastseg);
		}else{
			nbseg = (taillef/1000) + 1;
		}
		do
		{
			read = recvfrom(desccli, &buffer2, sizeof(buffer2), 0,  (struct sockaddr *) &client, &taillecli);
			if( read <= 0 )
			{
				perror( "recvfrom() error \n" );
				return -1;
			}
			for(i=0;i<5;i++){
				buffseg[i]=buffer2[i];
			}
			seg=atoi(buffseg);
			for(i=5;i<1005;i++){
				fputc(buffer2[i],f);
			}
			//printf("segment n° %d received\n",seg);

			//acquittement des segments reçus
			strcpy(buffersyn, "ACK_");
			sprintf(str, "%d", seg);
			strcat(buffersyn, str);
			sendto(desccli, &buffersyn, sizeof(buffersyn), 0, (struct sockaddr *)&client, taillecli);
			//fin ack

		}
		while(seg<nbseg-1);
		if(lastseg!=0){
			read = recvfrom(desccli, &buffer2, sizeof(buffer2), 0,  (struct sockaddr *) &client, &taillecli);
			if( read <= 0 )
			{
				perror( "recvfrom() error \n" );
				return -1;
			}
			for(i=0;i<5;i++){
				buffseg[i]=buffer2[i];
			}
			seg=atoi(buffseg);

			do{	
				fputc(buffer2[i],f);
				i++;
			}
			while(i<lastseg+4);
			printf("segment lastseg n° %d received\n",seg);
			//acquittement des segments reçus
			strcpy(buffersyn, "ACK_");
			sprintf(str, "%d", seg);
			strcat(buffersyn, str);
			sendto(desccli, &buffersyn, sizeof(buffersyn), 0, (struct sockaddr *)&client, taillecli);
			//fin ack
		}
		fclose(f);
		return 0;
	}