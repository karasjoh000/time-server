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

#define PORT 49999

//TODO catch errors.

static pthread_cond_t workerReleaser;
static pthread_mutex_t workerlist;
static pthread_t *releaserThread;

typedef struct _worker {
	pid_t pid;
	struct _worker *next;
} Worker;

static Worker* workers;

void addWorker(Worker *ptr, Worker *new) {
	new->next  = ptr;
	ptr = new;
}

Worker *allocWorker(pid_t pid) {
	Worker* new = (Worker*) malloc(sizeof(Worker));
	new->pid = pid;
	return new;
}

Worker* releaseWorkers (Worker *ptr) {
	int stat;
	if(!ptr) return NULL;
	Worker *temp;
	if(!waitpid(ptr->pid, &stat, WNOHANG))  {
		temp = ptr->next;
		free(ptr);
		return releaseWorkers(temp);
	}
	temp = ptr;
	while(temp->next) {
		if(!waitpid(ptr->next->pid, &stat, WNOHANG)) {
			Worker *del = temp->next;
			temp = temp->next = ptr->next->next;
			free(del);
		}
	}
	return ptr;

}

void forceRelease(Worker* workers) {
	if (!workers) return;
	forceRelease(workers->next);
	free(workers);
}

void shutdownServer(int code) {
	printf("\nshutting down server\n");
	pthread_cancel(*releaserThread);
	free(releaserThread);
	forceRelease(workers);
	exit(0);
}

void releaser(void* p) {
	Worker *workers = (Worker*) p;
	while( 1) {
		pthread_mutex_lock(&workerlist);
		pthread_cond_wait(&workerReleaser, &workerlist);
		releaseWorkers(workers);
		pthread_mutex_unlock(&workerlist);
	}

}

void serveClient(int netfd) {
	char buffer[40];
	time_t current = time(NULL);
	strcpy(buffer, ctime(&current));
	write(netfd, buffer, strlen(buffer));
	close(netfd);
	exit(0);
}

void setServerAddress(struct sockaddr_in* servAddr) {
	//
	memset(servAddr, 0, sizeof(*servAddr));
	servAddr->sin_family = AF_INET;
	servAddr->sin_port = htons(PORT);
	servAddr->sin_addr.s_addr = htonl(INADDR_ANY);

	return;
}

void bindNameToSocket(int listenfd, struct sockaddr_in* servAddr) {
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
	workers = NULL;
	// make a seperate thread that will waitpid WNOHANG on the list of pid's.
	releaserThread = ( pthread_t* ) malloc( sizeof( pthread_t ));
	pthread_create(releaserThread, NULL, ( void* ) releaser, workers );

	// get a fd for socket
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servAddr;

	setServerAddress(&servAddr); //set address, port, and add. family.
	bindNameToSocket(listenfd, &servAddr); // bind address to socket.
	listen(listenfd, 1); // set queue limit for incoming connections

	int connectfd;
	unsigned int length = sizeof(struct sockaddr_in);
	struct sockaddr_in clientAddr;

	while (1) {

		pid_t pid;

		printf("listing on port %d\n", PORT);

		connectfd = accept( listenfd, /* accept client */
		     (struct sockaddr *) &clientAddr, &length);

		printf("accepted client\n");
		// create new process to serve client.
		if ( !( pid = fork() ) )  serveClient(connectfd);
		// close fd so client will recieve EOF when child finishes.
		close(connectfd);

		printf("process %d is serving client\n", pid);
		// lock list of pid's and add the new pid.
		pthread_mutex_lock(&workerlist);
		addWorker(workers, allocWorker(pid));
		pthread_mutex_unlock(&workerlist);
		// signal thread to kill zombie processes.
		pthread_cond_signal(&workerReleaser);

	}

	return 0;

}
