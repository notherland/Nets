#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
//46.147.148.167    192.168.0.101 
enum {BACKLOG = 10}; // how many pending connections queue will hold
const char *PORT = "7707";  // the port users will be connecting to
const int BUFSIZE = 4096;
const char* GET = "GET / HTTP/1.1\r\n";
const char* OK = "HTTP/1.1 200 OK\r\n\r\n";
const char* NOT_FOUND = "HTTP/1.1 404 NOT FOUND\r\n\r\n";

static int listenfd, clients[BACKLOG];

int SearchAnalyse(char *request){
	int fd = -1;
	strtok(request, "=");
	char *reqw = strtok(NULL, " \0");	
	
	if (strcmp(reqw, "FRKT") == 0){
		if ((fd = open ("Html/FRKT.html", O_RDONLY)) < 0)
			perror ("open");
	}else if (strcmp(reqw, "FOPF") == 0){
		if ((fd = open ("Html/FOPF.html", O_RDONLY)) < 0)
			perror ("open");
	}
	return fd;
}


const char * ReqAnalyse(char *request){
	int fd = -1;
	char *response = (char *)malloc(BUFSIZE);

	if (strlen(request) >= strlen(GET)){
		strtok(request, "/");
		char *reqw = strtok(NULL, " \r");
		char *reqs = NULL;

		if (strcmp(reqw, "HTTP/1.1") == 0){
			if ((fd = open ("Html/Hello.html", O_RDONLY)) < 0)
				perror ("open");
		} else if ((reqs = strchr(reqw, '?')) != NULL){
			fd = SearchAnalyse(reqs);
		} else if ((fd = open (reqw, O_RDONLY)) < 0){
				perror ("open");
		}

		if (fd == -1){
			strcat(response, NOT_FOUND);
			return response;
		}

		strcat(response, OK);
		int k = read(fd, response + strlen(OK), BUFSIZE);
		if(k < 0)
			perror ("read");
		close(fd);
		return response;
	}
	strcat(response, NOT_FOUND);
	return response;
}

void StartServer(){

	struct addrinfo hints, *addrlist = (struct addrinfo *)malloc(sizeof(struct addrinfo)), *curaddr;
	
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	if (getaddrinfo(NULL, PORT, &hints, &addrlist) != 0) {
		perror("getaddrinfo");
		exit(EXIT_FAILURE);
	}

	for (curaddr = addrlist; curaddr != NULL; addrlist = addrlist->ai_next){
		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		if (listenfd < 0){
			perror ("socket");
			continue;
		}
		if (bind(listenfd, curaddr->ai_addr, curaddr->ai_addrlen) == 0)
			break;
	}

	if (curaddr == NULL){
		perror("bind");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(addrlist);

	if (listen(listenfd, BACKLOG) == -1){
		perror ("listen");
	}

	for (int i = 0; i < BACKLOG; i++)
		clients[i] = -1;

	return;
}

void Server(){
	int i = 0;

	StartServer();

	printf ("waiting for connection......\n");

	for(;;){
		while(clients[i] != -1){
			i++;
			if (i > BACKLOG){
				printf("Too many clients\n");
				i = 0;
				continue;
			}
		}
		clients[i] = accept(listenfd, NULL, 0);
		if (clients[i] < 0){
			perror("accept");
			continue;
		}
		
		if (!fork()) {
			close(listenfd);
			char request[BUFSIZE];
			
			int numbytes = recv(clients[i], request, sizeof(request), 0);
			if (numbytes < 0){
				perror("recv");
				continue;
			}
			if (numbytes == 0){
				printf("%d connection failed", i);
				clients[i] = -1;
				exit(0);
			}

			printf ("------------------------\nRequest: \n%s\n--------------------------\n", request);
			
			const char *response = ReqAnalyse(request);
			printf("-----------------------\nResponse:\n%s\n---------------------------\n", response);
			if(send(clients[i], response, strlen(response), 0) == -1)
				perror("send");
			close(clients[i]);
			clients[i] = -1;
			exit(0);
		}
		close(clients[i]);
		clients[i] = -1;
	}
}


int main(){
	
	Server();
	return 0;
}