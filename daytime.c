/*
   John Karasev
   CS 360 Systems Programming
   WSUV Spring 2018
   -----------------------------------------------------
   Assignment #9:
   Using network sockets, make a server and client where client gets time
   from the server. This file is the implementation of the client side.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <err.h>
#include <errno.h>

#define PORT 49999
/* Configures the address of the server. */
void setConnectionAddress(struct sockaddr_in *servAddr, struct hostent* host) {
	// set to 0 to avoid unwanted configurations.
	memset( (struct servAddr*) servAddr, 0, sizeof(servAddr));
	servAddr->sin_family = AF_INET; //set family and port.
	servAddr->sin_port = htons(PORT);
	// set the server address that client will connect to.
	memcpy(&servAddr->sin_addr, *(host->h_addr_list), sizeof(struct in_addr));

}


int main (int argc, char** argv) {

	int socketfd = socket( AF_INET, SOCK_STREAM, 0); // get a socket fd.
	struct sockaddr_in servAddr;
	struct hostent* hostEntry;

	if( argc == 1 )
		errx(0, "please provide a hostname in arguments");


	if ( ( hostEntry = gethostbyname( argv[1] ) ) == NULL )  // get struct with host info.
		errx( 1, "no name associated with %s\n", argv[1]);

	setConnectionAddress(&servAddr, hostEntry);

	if ( connect(socketfd, (struct sockaddr *) &servAddr, /* Connect to server */
	            sizeof(servAddr)) == -1)
		errx( 1, "error connecting: %s", strerror(errno));

	char buffer[40];
	int charread;

	while( (charread = read(socketfd, buffer, 40)) != 0 ) {
		//buffer[charread] = '\0';
		printf("%s", buffer);
	}

}
