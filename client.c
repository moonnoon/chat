#include <signal.h>
#include <pthread.h>
#include "chat.h"

#define LISTEN_QUEUE 5

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int socksrv;
fd_set rfds, orfds;
char *UID;

static int		nsec;			/* #seconds betweeen each alarm */
static int		maxnprobes;		/* #probes w/no response before quit */
static int		nprobes;		/* #probes since last server response */

struct online
{
	char uid[10];
};
static int nUser;

struct online *user;
//to python
char pyUser[BUFSIZE];
char ret[BUFSIZE];
char msg[BUFSIZE*20];

//create user list
int getUser(char *p)
{
	//printf("%s\n", p);
	struct online *pUser, *pExchange;
	int i;
	char *tmp = &p[4];
	memcpy(&nUser, p, 4);
	pUser = (struct online*)malloc(sizeof(struct online)*nUser);
	p = &p[4];
	memcpy(pyUser, p, sizeof pyUser-4);
	for (i = 0; i < nUser; i++) {
		tmp = strsep(&p, ":");
		memcpy(pUser[i].uid, tmp, 10);
		//printf("%d:%s\n", i, tmp);
	}
	pExchange = user;
	user = pUser;
	free(pExchange);
	return 0;
}
void sig_alrm(int signo)
{
	int n;
	char ch = HEART;
	if (++nprobes > maxnprobes) {
		fprintf(stderr, "server is unreachable\n");
		exit(0);
	}
	n = send(socksrv, &ch, BUFSIZE, MSG_OOB);
	if (n < 0) {
		perror("send");
		exit(0);
	}
	alarm(nsec);
	return;
}

void heartbeat_cli(int nsec_arg, int maxnprobes_arg)
{
	if ( (nsec = nsec_arg) < 1)
		nsec = 1;
	if ( (maxnprobes = maxnprobes_arg) < nsec)
		maxnprobes = nsec;
	nprobes = 0;

	fcntl(socksrv, F_SETOWN, getpid());

	signal(SIGALRM, sig_alrm);
	alarm(nsec);
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

int getPos(char *uid)
{
	int i;
	for (i = 0; i < nUser; i++) {
		if (!strcmp(user[i].uid, uid)) {
			return i;
		}
	}
	return -1;
}

/*

int main(int argc, const char *argv[])
{
	char buf[BUFSIZE], bufmsg[BUFSIZE];
	int socksrv;
	struct sockaddr_in server_addr;
	int bytes;
	//select 
	fd_set rfds, orfds;
	char uid[10];
	char *pTmp, *p;
	int i;

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
	inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
	connect(socksrv, (struct sockaddr *)(&server_addr), sizeof server_addr);

	//1 bytes for LOGIN
	buf[0] = LOGIN;
	sprintf(&buf[1], "%s", argv[1]);
	bytes = send(socksrv, buf, sizeof buf, 0);
	if (bytes < 0) {
		perror("send");
		return 1;
	}
	heartbeat_cli(1, 3);
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
			if (HEART == buf[0]) {
				getUser(&buf[1]);
			}
			else if (MESSAGE == buf[0]) {
				memcpy(uid, &buf[1], sizeof uid);
				pTmp = &buf[21];
				printf("%s %s\n%s\n", &buf[11], uid, pTmp);
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
			bufmsg[0] = MESSAGE;
			memcpy(&bufmsg[1], argv[1], 10);
			memcpy(&bufmsg[11], p, 10);
			memcpy(&bufmsg[21], pTmp, BUFSIZE-21);
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

*/

//For python call
int init()
{
	struct sockaddr_in server_addr;
	int ret;
	socksrv = socket(PF_INET, SOCK_STREAM, 0);
	if (socksrv < 0) {
		perror("socket");
		return 1;
	}
	setReuseAddr(socksrv);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
	ret = connect(socksrv, (struct sockaddr *)(&server_addr), sizeof server_addr);
	if (ret < 0) {
		perror("connect");
		return 1;
	}

	printf("finished\n");
	return 0;
}

int login(char *uid)
{
	int bytes;
	char buf[BUFSIZE];
	UID = strdup(uid);
	buf[0] = LOGIN;
	sprintf(&buf[1], "%s", uid);
	bytes = send(socksrv, buf, sizeof buf, 0);
	if (bytes < 0) {
		perror("send");
		return 1;
	}
	heartbeat_cli(1, 3);
	FD_ZERO(&orfds);
	//FD_SET(STDIN_FILENO, &orfds);
	FD_SET(socksrv, &orfds);
	return 0;
}

int sign_in(char *uid, char *password)
{
	return 0;
}

char *getUsers()
{
	return pyUser;
}

void main_loop()
{
	char buf[BUFSIZE], bufmsg[BUFSIZE];
	int bytes;
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
			if (HEART == buf[0]) {
				getUser(&buf[1]);
			}
			else if (MESSAGE == buf[0]) {
				//memcpy(uid, &buf[1], sizeof uid);
				//pTmp = &buf[21];
				sprintf(bufmsg, "%s %s\n%s\n", &buf[11], &buf[1], &buf[21]);
				//printf("pos %d\n", getPos(&buf[1]));
				printf("recv\n");
				int pos = getPos(&buf[1]);
				//memcpy(user[pos].pMsg, bufmsg, BUFSIZE);
				//user[pos].flag = 1;
			}
			nprobes = 0;
		}
	}
}

int sendMSG(char *receiver, char *msg)
{
	printf("start send\n");
	int bytes;
	char buf[BUFSIZE];
	bytes = strlen(msg);
	buf[0] = MESSAGE;
	memcpy(&buf[1], UID, 10);
	memcpy(&buf[11], receiver, 10);
	memcpy(&buf[21], msg, BUFSIZE-21);
	bytes = send(socksrv, buf, sizeof buf, 0);
	if (bytes < 0) {
		perror("send");
		return 1;
	}
	return 0;
}

char *haveMSG()
{
}

char *getMSG(char *uid)
{
}
