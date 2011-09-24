#ifndef IRCSOCK_H
#define IRCSOCK_H

#include "cbuffer.h"

typedef struct {
	char *host;
	int port;

	char *nick;
	char *chan;

	CBuffer *cbuf;
} IRCSock;

IRCSock *ircsock_create(char *host, int port, char *nick, char *chan);
void ircsock_free(IRCSock *ircsock);

#endif /* IRCSOCK_H */
