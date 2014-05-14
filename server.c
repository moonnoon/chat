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

#include "list.h"


#define BUFSIZE 1024
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

	setnonblocking(listen_fd);

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
	char reqBuf[BUFSIZE];
	int result;
	struct sockaddr_in client_addr;
	char *ip, *port;

	//get cilent addr
	socklen_t len = sizeof client_addr;
	result = getpeername(p_so->fd, (struct sockaddr*)&client_addr, &len);
	if (result != 0) {
		perror("getpeername");
		exit(1);
	}
	ip = inet_ntoa(client_addr.sin_addr);
	port = (char*)malloc(8);
	sprintf(port, "%d", ntohs(client_addr.sin_port));

	if((result = read(p_so->fd, reqBuf, sizeof reqBuf)) <= 0)
	{
		//client close connection;
		close(p_so->fd);
		hlist_del(&p_so->hlist);
		free(p_so);

		/*
		   struct user *pos, *n;
		   list_for_each_entry_safe(pos, n, &ulist, list)
		   if (!strcmp(pos->ip, ip) && !strcmp(pos->port, port)) {
		   printf("goodbye %s\n", pos->name);
		   list_del(&pos->list);
		   }
		   */

		if(result<0 && errno != ECONNRESET)
			bail("read()");

		return 0;
	} 
	reqBuf[result] = '\0';
	printf("%s", reqBuf);


	//add to user list
	//split string
	struct user tmp;
	tmp.name = reqBuf;
	tmp.ip = ip; 
	tmp.port = port;
	printf("port %s\n", tmp.port);
	list_add_tail(&tmp.list, &ulist);
	struct user *t;
	list_for_each_entry(t, &ulist, list)
	{
		printf("%s\n", t->name);
		printf("what???\n");
	}

	/*
	else {
		reqBuf[result] = '\0';
		printf("reqBuf: %s\n", reqBuf);
		result = write(p_so->fd, str, strlen(str));
		if(result < 0)
			bail("write()");
	}
	*/
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

	printf("create_conn ------------------\n");

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

	list_empty(&ulist);

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
