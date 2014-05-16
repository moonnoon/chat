#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <sys/epoll.h>
#include <signal.h>

#include "list.h"
#include "chat.h"


#define MAXCONN 20
#define MAX_EVENTS MAXCONN
#define LISTEN_QUEUE 20
#define PORT "8873"

struct sockfd_opt
{
	int fd;
	int (*do_task)(struct sockfd_opt *p_so);
	struct hlist_node hlist;
};

int send_reply(struct sockfd_opt *);
int create_conn(struct sockfd_opt *);
int init(int );
int intHash(int );
void setnonblocking(int );

struct hlist_head fd_hash[MAXCONN];
int epfd;
int num;
struct epoll_event *events;

LIST_HEAD(ulist);

static void bail(const char *on_what)
{
	fputs(strerror(errno), stderr);
	fputs(": ", stderr);
	fputs(on_what, stderr);
	fputc('\n', stderr);
	exit(1);
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

int main(int argc, const char *argv[])
{
	int listen_fd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in server_addr;
	socklen_t addr_len;
	int nev;
	unsigned int hash;
	struct hlist_node *pos, *n;
	struct sockfd_opt *p_so;

	int i;

	epfd = epoll_create(MAXCONN);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if((getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", strerror(errno));
		exit(1);
	}
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if((listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			fprintf(stderr, "Socket error: %s\a\n", strerror(errno));
			continue;
		}
		if(bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1)
		{
			fprintf(stderr, "Bind error: %s\n", strerror(errno));
			continue;
		}
		break;
	}//end for

	setReuseAddr(listen_fd);

	if(p == NULL)
	{
		fprintf(stderr, "socket error: %s\n", strerror(errno));
		exit(1);
	}

	freeaddrinfo(servinfo);

	if(listen(listen_fd, LISTEN_QUEUE) == -1)
	{
		fprintf(stderr, "Listen error: %s\n", strerror(errno));
		exit(1);
	}

	if(init(listen_fd))
		bail("init");

	events = malloc(sizeof(struct epoll_event)*MAX_EVENTS);
	if(events == NULL)
		bail("malloc");

	printf("Server is waiting for acceptance of new client...\n");
	for(;;)
	{
		nev = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if(nev < 0)
		{
			if(errno == EINTR)
				continue;
			free(events);
			bail("epoll_wait");
		}

		for (i = 0; i < nev; i++) {
			hash = intHash(events[i].data.fd)&MAXCONN;
			//printf("hash use: %d\n", hash);
			hlist_for_each_entry_safe(p_so, pos, n, &fd_hash[hash], hlist)
			{
				if(p_so->fd == events[i].data.fd)
				{
					p_so->do_task(p_so);
				}
			}
		}
	}

	close(listen_fd);

	return 0;
}


int send_reply(struct sockfd_opt *p_so)
{
	char buf[BUFSIZE], bufmsg[BUFSIZE];
	char *p;
	int result;
	struct sockaddr_in client_addr;
	struct user *pos, *n;
	char *ip;
	int port;
	char uid[10];

	//get cilent addr
	socklen_t len = sizeof client_addr;
	result = getpeername(p_so->fd, (struct sockaddr*)&client_addr, &len);
	ip = inet_ntoa(client_addr.sin_addr);
	port = ntohs(client_addr.sin_port);
	if((result = recv(p_so->fd, buf, sizeof buf, 0)) <= 0)
	{
		//client close connection;
		close(p_so->fd);
		hlist_del(&p_so->hlist);
		free(p_so);

		list_for_each_entry_safe(pos, n, &ulist, list)
			if (!strcmp(pos->ip, ip) && pos->port == port) {
				printf("goodbye %s\n", pos->uid);
				free(pos->uid);
				free(pos->ip);
				list_del(&pos->list);
			}
		if(result<0 && errno != ECONNRESET)
			bail("recv()");
		return 0;
	} 
	if (!memcmp(HEART, buf, 3)) {
		memset(buf, 0, sizeof buf);
		int i = 0;
		// 3 bytes for the online user;
		p = &buf[4];
		list_for_each_entry(pos, &ulist, list)
		{
			if (strcmp(pos->uid, buf)) {
				//sprintf(p, "%s,%s,%s:", pos->uid, pos->ip, pos->port);
				sprintf(p, "%s:", pos->uid);
				p += strlen(p);
				i++;
			}
			//printf("%s\n", pos->uid);
			//printf("port: %s\n", pos->port);
		}
		memcpy(buf, &i, 4);
		result = send(p_so->fd, buf, sizeof buf, 0);
		if (result < 0) {
			perror("send");
		}
	}

	else if (!memcmp(LOGIN, buf, 3)) {
		p = &buf[3];
		buf[result] = '\0';
		printf("%s login\n", p);

		//add to user list
		struct user *pTmp = (struct user*)malloc(sizeof(struct user));
		pTmp->uid = strdup(p);
		pTmp->ip = strdup(ip); 
		pTmp->port = port;
		pTmp->fd = p_so->fd;
		list_add_tail(&pTmp->list, &ulist);
	}

	else if (!memcmp(MESSAGE, buf, 3)) {
		memcpy(uid, &buf[3], sizeof uid);
		list_for_each_entry(pos, &ulist, list)
		{
			if (!strncmp(pos->uid, uid, sizeof uid)) {
				break;
			}
		}
		memcpy(bufmsg, MESSAGE, 3);
		memcpy(&bufmsg[3], &buf[13], 10);
		memcpy(&bufmsg[13], &buf[23], BUFSIZE - 23);
		result = send(pos->fd, bufmsg, sizeof bufmsg, 0);
		if (result < 0) {
			perror("send");
		}
	}
	return 0;
}


int create_conn(struct sockfd_opt *p_so)
{
	struct sockaddr_in client_addr;
	int conn_fd;
	socklen_t sin_size;
	unsigned int hash;
	struct epoll_event ev, evTest;
	int ret;

	sin_size = sizeof(struct sockaddr_in);

	if((conn_fd = accept(p_so->fd, (struct sockaddr*)&client_addr, &sin_size)) == -1)
	{
		fprintf(stderr, "Accept error %s:\n", strerror(errno));
		exit(1);
	}

	setnonblocking(conn_fd);
	fprintf(stdout, "Server got connection from %s : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	if((p_so=malloc(sizeof(struct sockfd_opt))) == NULL)
	{
		perror("malloc");
		return -1;
	}

	p_so->fd = conn_fd;
	p_so->do_task = send_reply;

	hash = intHash(conn_fd) & MAXCONN;
	//printf("hash add: %d\n", hash);
	hlist_add_head(&p_so->hlist, &fd_hash[hash]);
	num++;

	ev.data.fd = conn_fd;
	//ev.events = EPOLLIN | EPOLLET;
	ev.events = EPOLLIN;
	//printf("%x\n", ev.events);
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev);
	if (ret) {
		perror("epoll_ctl");
		return -1;
	}

	return 0;
}


int init(int fd)
{
	struct sockfd_opt *p_so;
	struct epoll_event ev;
	int ret;
	unsigned int hash;

	//init hashtable
	hlist_empty(&fd_hash[0]);
	num = 0;
	if((p_so = malloc(sizeof(struct sockfd_opt))) == NULL) {
		perror("malloc");
		return -1;
	}

	p_so->fd = fd;
	p_so->do_task = create_conn;

	hash = intHash(fd)&MAXCONN;
	//printf("hash add: %d\n", hash);
	hlist_add_head(&p_so->hlist, &fd_hash[hash]);
	ev.data.fd = fd;
	//ev.events = EPOLLIN | EPOLLET;
	ev.events = EPOLLIN;

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
	if (ret) {
		bail("epoll_ctl");
	}
	num++;

	return 0;
}


int intHash(int key)
{
	key += ~(key<<15);
	key ^= (key>>10);
	key += (key<<3);
	key ^= (key>>6);
	key += ~(key<<11);
	key ^= (key>>16);
	key %= MAXCONN;

	return key;
}


void setnonblocking(int sock)
{
	int opts;

	opts = fcntl(sock, F_GETFL);
	if(opts < 0)
		bail("fcntl");
	opts = opts|O_NONBLOCK;
	if(fcntl(sock, F_SETFL, opts))
		bail("fcntl");
}
