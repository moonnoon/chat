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

struct onlineusers
{
	char uid[10];
	int flag;
	struct list_head lmsg;
	struct list_head list;
};
LIST_HEAD(ulist);
struct msg
{
	char buf[BUFSIZE];
	struct list_head list;
};

//to python
char ret[BUFSIZE];
char msg[BUFSIZE*20];

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

int initUsers(char *uids)
{
	char *tmp;
	struct onlineusers *user;
	printf("init %s\n", uids);
	while (uids!=NULL) {
		tmp = strsep(&uids, ":");
		user = (struct onlineusers*)malloc(sizeof(struct onlineusers));
		memcpy(user->uid, tmp, 10);
		INIT_LIST_HEAD(&user->lmsg);
		user->flag = 0;
		list_add_tail(&user->list, &ulist);
	}
	return 0;
}

int addUser(char *uid)
{
	struct onlineusers *user = (struct onlineusers*)malloc(sizeof(struct onlineusers));
	memcpy(user->uid, uid, 10);
	INIT_LIST_HEAD(&user->lmsg);
	user->flag = 0;
	list_add_tail(&user->list, &ulist);
	return 0;
}

int delUser(char *uid)
{
	printf("del......%s\n", uid);
	pthread_mutex_lock(&lock);
	struct onlineusers *pos, *n;
	struct msg *p, *m;
	list_for_each_entry_safe(pos, n, &ulist, list)
	{
		if(!strcmp(pos->uid, uid)) {
			list_del(&pos->list);
			break;
		}
	}
	if (pos->flag) {
		list_for_each_entry_safe(p, m, &pos->lmsg, list)
		{
			list_del(&p->list);
			free(p);
		}
	}
	free(pos);
	//printf("segmentation fault\n");
	pthread_mutex_unlock(&lock);
	printf("what falut????\n");
	return 0;
}

inline struct onlineusers *getUser(char *uid)
{
	struct onlineusers *pos, *n;
	list_for_each_entry_safe(pos, n, &ulist, list)
	{
		if (!strcmp(pos->uid, uid)) {
			return pos;
		}
	}
	return NULL;
}

int addMsg(char *uid, char *pMsg)
{
	pthread_mutex_lock(&lock);
	struct onlineusers *user;
	struct msg *m = (struct msg*)malloc(sizeof(struct msg));
	memcpy(m->buf, pMsg, BUFSIZE);
	user = getUser(uid);
	list_add_tail(&m->list, &user->lmsg);
	user->flag = 1;
	pthread_mutex_unlock(&lock);
	return 0;
}

int haveMsg(char *uid)
{
	if (getUser(uid)->flag) {
		return 1;
	}
	return 0;
}

//must judge haveMsg before call it
char *getMsg(char *uid)
{
	pthread_mutex_lock(&lock);
	struct onlineusers *user;
	struct msg *m;
	user = getUser(uid);
	m = list_entry(user->lmsg.next, struct msg, list);
	memcpy(ret, m->buf, BUFSIZE);
	list_del(&m->list);
	free(m);
	if (list_empty(&user->lmsg)) {
		user->flag = 0;
	}
	pthread_mutex_unlock(&lock);
	return ret;
}

//return users who send current user
char *remind()
{
	pthread_mutex_lock(&lock);
	char *p;
	char tmp[11];
	struct onlineusers *pos, *n;
	memset(ret, 0, sizeof ret);
	p = ret;
	list_for_each_entry_safe(pos, n, &ulist, list)
	{
		if(pos->flag)
		{
			sprintf(tmp, "%s:", pos->uid);
			memcpy(p, tmp, 10);
			p += strlen(tmp);
		}
	}
	pthread_mutex_unlock(&lock);
	return ret;
}

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
		return -1;
	}

	//heartbeat_cli(1, 3);
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
	struct onlineusers *pos, *n;
	char tmp[11];
	char *p;
	//no one online
	if (list_empty(&ulist)) {
		return NULL;
	}
	memset(ret, 0, sizeof ret);
	p = ret;
	list_for_each_entry_safe(pos, n, &ulist, list)
	{
		sprintf(tmp, "%s:", pos->uid);
		memcpy(p, tmp, 10);
		p += strlen(tmp);
	}
	*(p-1) = '\0';
	//printf("%s\n", ret);
	return ret;
}

void main_loop()
{
	char buf[BUFSIZE], bufmsg[BUFSIZE];
	int bytes;
	struct onlineusers *pUser;
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
				//getUser(&buf[1]);
			}
			else if (MESSAGE == buf[0]) {
				memset(bufmsg, 0, sizeof bufmsg);
				sprintf(bufmsg, "%s %s\n%s\n", &buf[11], &buf[1], &buf[21]);
				addMsg(&buf[1], bufmsg);
				printf("recv:%s\n", bufmsg);
				//printf("get %s\n", getMsg(&buf[1]));
				printf("finish recv\n");
			}
			else if (LOGIN == buf[0]) {
				initUsers(&buf[1]);
			}
			else if (ONLINE == buf[0]) {
				addUser(&buf[1]);
			}
			else if (OFFLINE == buf[0]) {
				delUser(&buf[1]);
			}
			nprobes = 0;
		}
	}
}

int sendMSG(char *receiver, char *msg)
{
	printf("%s\n", msg);
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

void clean()
{
	close(socksrv);
}
