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
#define FREADSIZE 50

int main {
	
	int taillef;
	char contenuBuff[FREADSIZE][TABSIZE-6]; //taleau contenant les 50 segments courants
	
	int cptBuff  =0;
	int nbBytes = 0;
	
	
	
	FILE* f = NULL;
	f = fopen("sample4_l.jpg","r");
	if (f != NULL){
			
	}else{
		printf("Impossible d'ouvrir le fichier %s\n",nomf);
	}
	
	
	FILE* f2 = NULL;
	f2 = fopen("sample4_l.jpg","w+");
	if (f != NULL){	
	}else{
		printf("Impossible d'ouvrir le fichier 2 %s\n",nomf);
	}
	
	taillef = getTaille(f);
	
	//calcul du nombre de segments requis
	if(taillef%1400!=0){
		nbseg = (taillef/1400)+1;
	}else{
		nbseg = (taillef/1400);
	}
	
	
	for (cptBuff = 0; cptBuff < FREADSIZE ; cptBuff++){
		if ( (nbBytes = fread(contenuBuff[cptBuff],sizeof(char),TABSIZE-6,f))<0){	
			perror ("Erreur copie octets\n");
		}
	}
	
	for (cptBuff = 0; cptBuff < FREADSIZE ; cptBuff++){
		
	}
	
	
	
	
return 0;
}
