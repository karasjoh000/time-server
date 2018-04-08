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

#define PORT 49999

//TODO catch errors.

void setConnectionAddress(struct sockaddr_in *servAddr, struct hostent* host) {
	memset( (struct servAddr*) servAddr, 0, sizeof(servAddr));
	servAddr->sin_family = AF_INET;
	servAddr->sin_port = htons(PORT);
	/* this is magic, unless you want to dig into the man pages */
	struct in_addr **pptr = (struct in_addr **) host->h_addr_list;
	memcpy(&servAddr->sin_addr, *pptr, sizeof(struct in_addr));

}


int main (int argc, char** argv) {

	int socketfd = socket( AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servAddr;
	struct hostent* hostEntry;

	if ( ( hostEntry = gethostbyname( argv[1] ) ) == NULL )
		errx( 1, "no name associated with %s\n", argv[1] );

	setConnectionAddress(&servAddr, hostEntry);

	connect(socketfd, (struct sockaddr *) &servAddr,
	            sizeof(servAddr));

	char buffer[20];
	int charread;

	while( (charread = read(socketfd, buffer, 19)) != 0 ) {
		//buffer[charread+1] = '\0';
		printf("%s", buffer);
	}

}
