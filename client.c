#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define MAXBUFLEN 1024
#define SERV_PORT 8873
#define SERVER_IP "192.168.0.1"
#define LISTEN_QUEUE 5

struct sockaddr_in local_addr;
int sockfd;
//int sockmsg;
int local_port;


void setReuseAddr(int sock)
{
	int ret;
	int so_reuseaddr = 1;
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof so_reuseaddr);
	if (ret) {
		perror("setsockopt");
	}
}
void *recvMsg(void *arg)
{
	struct sockaddr_in listen_addr, talk_addr;
	int ret = 1;
	int sockmsg, new_fd;
	char buf[MAXBUFLEN];
	socklen_t listen_len = sizeof listen_addr;
	sockmsg = socket(AF_INET, SOCK_STREAM, 0);
	if(sockmsg != 0) {
		perror("socket");
		exit(1);
	}
	local_port = ntohs(local_addr.sin_port);
	talk_addr.sin_family = AF_INET;
	talk_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	while (ret != 0) {
		local_port++;
		ret = bind(sockmsg, (struct sockaddr*)&talk_addr, sizeof(talk_addr));
	}
	ret = listen(sockmsg, LISTEN_QUEUE);
	if (ret != 0) {
		perror("listen");
		exit(1);
	}
	new_fd = accept(sockmsg, (struct sockaddr*)&listen_addr, &listen_len);
	for(;;)
	{
		memset(buf, 0, sizeof buf);
		ret = recv(new_fd, buf, sizeof buf, 0);
		printf("%s\n", buf);
	}
	printf("Hello recv\n");
}

void *sendMsg(void *arg)
{
	int sockmsg;
	struct sockaddr_in listen_addr;
	char *name = (char*)arg;
	sockmsg = socket(AF_INET, SOCK_STREAM, 0);
	setReuseAddr(sockfd);
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, "127.0.0.1", &listen_addr.sin_addr);
	connect(sockfd, (struct sockaddr *)(&listen_addr), sizeof listen_addr);
	printf("hello send\n");
}


void tcpClient()
{
}

void udpClient(char *msg)
{
	int sockfd, numbytes;
	char buf[MAXBUFLEN];
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if((rv = getaddrinfo(NULL, "8873", &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo %s\n", gai_strerror(rv));
		return;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			close(sockfd);
			perror("talker: connect");
			continue;
		}
		break;
	}

	if(p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return;
	}

	freeaddrinfo(servinfo);

	if((numbytes = sendto(sockfd, msg, strlen(msg), 0,
					p->ai_addr, p->ai_addrlen)) == -1)
	{
		perror("talker: sendto");
		return;
	}
	printf("talker: send %d bytes\n", numbytes);
	printf("%s\n", msg);
	close(sockfd);
}


int main(int argc, const char *argv[])
{
	char buf[1024];
	struct sockaddr_in server_addr;
	socklen_t len = sizeof local_addr;
	int bytes;

	pthread_t nRecv, nSend;

	if (argc != 2) {
		printf("usage: \"name\"\n");
		return 0;
	}
	printf("welcome %s\n", argv[1]);
	
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	setReuseAddr(sockfd);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
	connect(sockfd, (struct sockaddr *)(&server_addr), sizeof server_addr);
	if(getsockname(sockfd, (struct sockaddr *)(&local_addr), &len) < 0)
	{
		perror("getsockname");
		exit(1);
	}
	printf("len = %d\n", len);
	printf("%s, %d\n", inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port));
	printf("len = %d\n", strlen(inet_ntoa(local_addr.sin_addr)));

	/*
	if(pthread_create(&nRecv, NULL, recvMsg, NULL) != 0)
	{
		perror("pthread_create");
		exit(1);
	}
	if(pthread_create(&nSend, NULL, sendMsg, "aaa") != 0)
	{
		perror("pthread_create");
		exit(1);
	}
	*/
	bytes = send(sockfd, argv[1], strlen(argv[1]), 0);
	if (bytes < 0) {
		perror("send");
		return 1;
	}
	for(;;)
	{
		sleep(10000);
		fgets(buf, sizeof buf, stdin);
		bytes = send(sockfd, buf, strlen(buf), 0);
		if (bytes < 0) {
			perror("send");
			return 1;
		}

		printf("send %d bytes!\n", bytes);
		memset(buf, 0, sizeof buf);
	}
	/*
	bytes = recv(sockfd, buf, sizeof(buf), 0);
	printf("receive %d bytes!\n", bytes);
	buf[bytes] = '\0';
	printf("%s\n", buf);
	*/



	close(sockfd);
	return 0;
}
