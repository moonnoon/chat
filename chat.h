#ifndef _CHAT_H
#define _CHAT_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "list.h"

//packet
#define SIGN_IN	'1'
#define LOGIN 	'2'
#define HEART	'3'
#define ONLINE	'4'
#define OFFLINE	'5'
#define MESSAGE	'6'
#define ERROR	'7'

//Error types
#define UID_EXIST	'A'


#define BUFSIZE 1024
#define SERV_PORT 8873
//#define SERVER_IP "192.168.208.1"
#define SERVER_IP "127.0.0.1"

#endif
