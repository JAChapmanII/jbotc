#ifndef IRCSOCK_H
#define IRCSOCK_H

#include "cbuffer.h"

typedef struct {
	char *host;
	int port;
	int domain;

	char *nick;
	char *chan;

	int socket;

	CBuffer *cbuf;
} IRCSock;

IRCSock *ircsock_create(char *host, int port, char *nick, char *chan);
void ircsock_free(IRCSock *ircsock);

int ircsock_connect(IRCSock *ircsock);
void ircsock_read(IRCSock *ircsock);
void ircsock_send(IRCSock *ircsock, char *str);

int ircsock_join(IRCSock *ircsock);

#endif /* IRCSOCK_H */
