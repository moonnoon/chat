#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "chat.h"

#define LISTEN_QUEUE 5

struct sockaddr_in local_addr;
//int sockmsg;

static int		servfd;
static int		nsec;			/* #seconds betweeen each alarm */
static int		maxnprobes;		/* #probes w/no response before quit */
static int		nprobes;		/* #probes since last server response */
static void	sig_urg(int), sig_alrm(int);

struct online
{
	char *uid;
};
static int nUser;

struct online *user;

//create user list
int getUser(char *p)
{
	//printf("%s\n", p);
	free(user);
	int i;
	char *tmp = &p[4];
	memcpy(&nUser, p, 4);
	user = (struct online*)malloc(sizeof(struct online)*nUser);
	p = &p[4];
	for (i = 0; i < nUser; i++) {
		tmp = strsep(&p, ":");
		user[i].uid = tmp;
		printf("%s\n", tmp);
	}
	return 0;
}

void heartbeat_cli(int servfd_arg, int nsec_arg, int maxnprobes_arg)
{
	servfd = servfd_arg;		/* set globals for signal handlers */
	if ( (nsec = nsec_arg) < 1)
		nsec = 1;
	if ( (maxnprobes = maxnprobes_arg) < nsec)
		maxnprobes = nsec;
	nprobes = 0;

	fcntl(servfd, F_SETOWN, getpid());

	signal(SIGALRM, sig_alrm);
	alarm(nsec);
}

static void sig_alrm(int signo)
{
	int n;
	if (++nprobes > maxnprobes) {
		fprintf(stderr, "server is unreachable\n");
		exit(0);
	}
	n = send(servfd, HEART, sizeof HEART, MSG_OOB);
	if (n < 0) {
		perror("send");
		exit(0);
	}
	alarm(nsec);
	return;					/* may interrupt client code */
}


void setReuseAddr(int sock)
{
	int ret;
	int so_reuseaddr = 1;
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof so_reuseaddr);
	if (ret) {
		perror("setsockopt");
	}
}

/*
void *recvMsg(void *arg)
{
	struct sockaddr_in listen_addr, talk_addr;
	int ret = 1;
	int sockmsg, new_fd;
	char buf[BUFSIZE];
	socklen_t len;
	socklen_t listen_len = sizeof listen_addr;
	sockmsg = socket(AF_INET, SOCK_STREAM, 0);
	printf("0\n");
	if(sockmsg < 0) {
		perror("socket");
		exit(1);
	}
	talk_addr.sin_port = ntohs(0);
	talk_addr.sin_family = AF_INET;
	talk_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(sockmsg, (struct sockaddr*)&talk_addr, sizeof(talk_addr));
	if (ret != 0) {
		perror("bind");
		exit(1);
	}
	len = sizeof local_addr;
	memset(&local_addr, 0, len);
	if(getsockname(sockmsg, (struct sockaddr *)(&local_addr), &len) < 0)
	{
		perror("getsockname");
		exit(1);
	}
	ret = listen(sockmsg, LISTEN_QUEUE);
	if (ret != 0) {
		perror("listen");
		exit(1);
	}
	new_fd = accept(sockmsg, (struct sockaddr*)&listen_addr, &listen_len);
	printf("real\n");
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
	int i;
	struct sockaddr_in listen_addr;
	char *uid;
	char buf[BUFSIZE];
	for (i = 0; i < nUser; i++) {
		printf("%s\n", user[i].uid);
	}
	printf("is online\n");
	uid = (char*)malloc(sizeof(char)*20);
	fgets(uid, sizeof uid, stdin);
	for (i = 0; i < nUser; i++) {
		if (!strncmp(uid, user[i].uid, strlen(uid)-1)) {
			printf("no break???\n");
			break;
		}
	}
	sockmsg = socket(AF_INET, SOCK_STREAM, 0);
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(user[i].port);
	inet_pton(AF_INET, user[i].ip, &listen_addr.sin_addr);
	connect(sockmsg, (struct sockaddr *)(&listen_addr), sizeof listen_addr);
	printf("connect\n");
	for(;;)	{
		fgets(buf, sizeof buf, stdin);
		send(sockmsg, buf, strlen(buf), 0);
	}
}


void udpClient(char *msg)
{
	int sockfd, numbytes;
	char buf[BUFSIZE];
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

*/


int main(int argc, const char *argv[])
{
	char buf[BUFSIZE], bufmsg[BUFSIZE];
	int socksrv;
	struct sockaddr_in server_addr;
	socklen_t len = sizeof local_addr;
	int bytes;
	//select 
	fd_set rfds, orfds;
	char uid[10];
	char *pTmp, *p;
	int i;

	pthread_t nRecv, nSend;

	if (argc != 2) {
		printf("usage: \"uid\"\n");
		return 0;
	}
	memcpy(uid, argv[1], sizeof uid);
	printf("welcome %s\n", uid);
	
	socksrv = socket(PF_INET, SOCK_STREAM, 0);
	setReuseAddr(socksrv);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
	connect(socksrv, (struct sockaddr *)(&server_addr), sizeof server_addr);
	/*
	if(getsockname(socksrv, (struct sockaddr *)(&local_addr), &len) < 0)
	{
		perror("getsockname");
		exit(1);
	}
	*/
	//printf("%s, %d\n", inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port));

	//3 bytes for LOGIN
	//4 bytes for port 
	sprintf(buf, "%s", LOGIN);
	sprintf(&buf[3], "%s", argv[1]);
	bytes = send(socksrv, buf, sizeof buf, 0);
	if (bytes < 0) {
		perror("send");
		return 1;
	}
	heartbeat_cli(socksrv, 3, 3);
	FD_ZERO(&orfds);
	FD_SET(STDIN_FILENO, &orfds);
	FD_SET(socksrv, &orfds);
	for(;;)
	{
		rfds = orfds;
		memset(buf, 0, sizeof buf);
		if (select(socksrv+1, &rfds, NULL, NULL, NULL) == -1) {
			if (errno == EINTR) {
				continue;
			}
			perror("select");
			break;
		}
		if (FD_ISSET(socksrv, &rfds)) {
			bytes = recv(socksrv, buf, sizeof buf, 0);
			if (bytes < 0) {
				perror("recv");
			}
			if (!memcmp(HEART, buf, 3)) {
				getUser(buf);
			}
			else if (!memcmp(MESSAGE, buf, 3)) {
				memcpy(uid, &buf[3], sizeof uid);
				pTmp = &buf[23];
				printf("%s %s\n%s\n", &buf[13], uid, pTmp);
			}
			nprobes = 0;
		}
		else if (FD_ISSET(STDIN_FILENO, &rfds)) {
			fgets(buf, sizeof buf, stdin);
			bytes = strlen(buf);
			if (bytes>0 && buf[--bytes]=='\n') {
				buf[bytes] = 0;
			}
			pTmp = buf;
			p = strsep(&pTmp, " ");
			memcpy(bufmsg, MESSAGE, 3);
			memcpy(&bufmsg[3], argv[1], 10);
			memcpy(&bufmsg[13], p, 10);
			memcpy(&bufmsg[23], pTmp, BUFSIZE-23);
			bytes = send(socksrv, bufmsg, sizeof bufmsg, 0);
			if (bytes < 0) {
				perror("send");
			}
			p =  NULL;
			pTmp = NULL;
		}
	}

	close(socksrv);
	return 0;
}
