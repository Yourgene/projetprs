CC= gcc
CFLAGS = -Wall -g

all: serveur clientudp 

clientudp: clientudp.o
	$(CC) -o clientudp clientudp.o $(CFLAGS)
	
clientudp.o: clientudp.c
	$(CC) -o clientudp.o -c clientudp.c  $(CFLAGS)

serveur: serveur.o
	$(CC) -o serveur serveur.o $(CFLAGS)
	
serveur.o: serveur.c
	$(CC) -o serveur.o -c serveur.c  $(CFLAGS)
	
clean:
	rm -f clientudp serveur *.o *~

