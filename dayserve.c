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

#define PORT 49999


int main (int argc, char** argv) {
	int socketfd;
	socketfd = socket( AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servAddr;
	struct hostent* hostEntry;
	struct in_addr **pptr;

	memset( &servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(PORT);

	hostEntry = gethostbyname(argv[1]);
	/* test for error using herror() */

	/* this is magic, unless you want to dig into the man pages */
	pptr = (struct in_addr **) hostEntry->h_addr_list;
	memcpy(&servAddr.sin_addr, *pptr, sizeof(struct in_addr));

	connect(socketfd, (struct sockaddr *) &servAddr,
	            sizeof(servAddr));

	char buffer[20];
	int charread;

	while( (charread = read(socketfd, buffer, 19)) != 0 ) {
		buffer[charread+1] = '\0';
		printf("%s", buffer);
	}

}
