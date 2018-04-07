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

#define PORT 49999

typedef struct _worker {
	pid_t pid;
	struct _worker *next;
} Worker;

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

void serveClient(int netfd) {
	sleep(2); 
	char buffer[20];
	strcpy(buffer, "hello world\n");
	write(netfd, buffer, strlen("hello world\n") + 1);
	close(netfd);
	exit(0);
}

void setServerAddress(struct sockaddr_in* servAddr) {

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

	Worker* workers = NULL;

	int listenfd;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in servAddr;

	setServerAddress(&servAddr);

	bindNameToSocket(listenfd, &servAddr);

	listen(listenfd, 1);

	int connectfd;
	unsigned int length = sizeof(struct sockaddr_in);
	struct sockaddr_in clientAddr;

	while (1) {
		pid_t pid;
		printf("listing on port %d\n", PORT);

		connectfd = accept( listenfd,
		     (struct sockaddr *) &clientAddr, &length);

		printf("accepted client\n");

		if ( !( pid = fork() ) )  serveClient(connectfd);

		close(connectfd);

		printf("process %d is serving client\n", pid);

		addWorker(workers, allocWorker(pid));

		releaseWorkers(workers);

	}

	return 0;

}
