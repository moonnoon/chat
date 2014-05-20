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

//packet
#define SIGN_IN	'1'
#define LOGIN 	'2'
#define HEART	'3'
#define MESSAGE	'4'
#define ERROR	'5'

//Error types
#define UID_EXIST	'1'
#define OFFLINE		'2'


#define BUFSIZE 1024
#define SERV_PORT 8873
//#define SERVER_IP "192.168.0.1"
#define SERVER_IP "127.0.0.1"

#endif
