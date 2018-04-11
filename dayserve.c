/*
   John Karasev
   CS 360 Systems Programming
   WSUV Spring 2018
   -----------------------------------------------------
   Assignment #9:
   Using network sockets, make a server and client where client gets time
   from the server. This file is the implementation of the server side.
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
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <err.h>
#include <errno.h>

#define PORT 49999

//TODO catch errors.

static pthread_cond_t workerReleaser;  // for the thread that kills off
static pthread_mutex_t m_workerReleaser; 				       // zombie processes.
static pthread_t *releaserThread;      // thread that kills zombie processes.

static int connectfd;


// SIGINT handler, releases mem and cancels second thread.
void shutdownServer(int code) {
	close(connectfd);
	printf("\nshutting down server\n");
	pthread_cancel(*releaserThread);
	free(releaserThread);
	exit(0);
}

void killZombies() {
	int stat;
	while (waitpid(-1, &stat, WNOHANG));
	return;
}

// releaserThread waiting function. When signaled, executes releaseWorkers().
void releaser(void* p) {
	while( 1) {
		pthread_mutex_lock(&m_workerReleaser);
		pthread_cond_wait(&workerReleaser, &m_workerReleaser);
		killZombies();
		pthread_mutex_unlock(&m_workerReleaser);
	}

}

// Forked process exectues this function, sends date/time to client then exits.
void serveClient(int netfd) {
	char buffer[40];
	time_t current = time(NULL);
	strcpy(buffer, ctime(&current));
	write(netfd, buffer, strlen(buffer) + 1); //+1 for null terminator.
	close(netfd);
	exit(0);
}

// configures server address (Port, family, and address).
void setServerAddress(struct sockaddr_in* servAddr) {
	memset(servAddr, 0, sizeof(*servAddr));
	servAddr->sin_family = AF_INET;
	servAddr->sin_port = htons(PORT);
	servAddr->sin_addr.s_addr = htonl(INADDR_ANY);

	return;
}

// when name is configured, it needs to be binded to the socket.
// listedfd -> socket fd
// servAddr -> struct that containes address configs.
void bindNameToSocket(int listenfd, struct sockaddr_in* servAddr) {
	int on = 1;
	// allow reuse of socket without waiting 3 min before using again.
	if ( setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0)
		errx( 1, "error on sockopt: %s", strerror(errno));
	if ( bind( listenfd, /*assigns a name to an unnamed socket*/
		(struct sockaddr *) servAddr, sizeof(*servAddr)) < 0) {
			perror("bind");
			exit(1);
	}

}

int main () {
	// cancel threads and release memory then exit, see shutdownServer.
	signal( SIGINT, shutdownServer );
	// init list of pid's.
	// make a seperate thread that will waitpid WNOHANG on the list of pid's.
	releaserThread = ( pthread_t* ) malloc( sizeof( pthread_t ));
	pthread_create(releaserThread, NULL, ( void* ) releaser, NULL);

	// get a fd for socket
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) errx( 1, "error getting socket fd: %s", strerror(errno));

	struct sockaddr_in servAddr;

	setServerAddress(&servAddr); //set address, port, and add. family.
	bindNameToSocket(listenfd, &servAddr); // bind address to socket.

	// set queue limit for incoming connections
	if(listen(listenfd, 1) == -1)
		errx( 1, "error setting connections queue: %s", strerror(errno));


	unsigned int length = sizeof(struct sockaddr_in);
	struct sockaddr_in clientAddr;

	while (1) {

		pid_t pid;

		printf("listing on port %d\n", PORT);

		if ( (connectfd = accept( listenfd, (struct sockaddr *) &clientAddr,
		      &length)) == -1 )
			errx( 1, "error connecting: %s", strerror(errno));

		char clienthost[INET_ADDRSTRLEN];
		if ( ( inet_ntop( AF_INET, &clientAddr.sin_addr,
		     clienthost, INET_ADDRSTRLEN ) ) == NULL)
			errx( 1, "error connecting: %s", strerror(errno));
		printf("accepted client from %s\n", clienthost);

		// create new process to serve client.
		if ( !( pid = fork() ) )  serveClient(connectfd);
		// close fd so client will recieve EOF when child finishes.
		close(connectfd);

		printf("process %d is serving client\n", pid);

		// signal thread to kill zombie processes.
		pthread_cond_signal(&workerReleaser);

	}

	return 0;

}
