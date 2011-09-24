#include "ircsock.h"
#include <stdlib.h>
#include <string.h>

IRCSock *ircsock_create(char *host, int port, char *nick, char *chan) {
	IRCSock *ircsock = malloc(sizeof(IRCSock));
	if(!ircsock)
		return NULL;

	ircsock->host = NULL;
	ircsock->nick = NULL;
	ircsock->chan = NULL;

	ircsock->host = malloc(strlen(host) + 1);
	if(!ircsock->host) {
		ircsock_free(ircsock);
		return NULL;
	}
	strcpy(ircsock->host, host);

	ircsock->port = port;

	ircsock->nick = malloc(strlen(nick) + 1);
	if(!ircsock->nick) {
		ircsock_free(ircsock);
		return NULL;
	}
	strcpy(ircsock->nick, nick);

	ircsock->chan = malloc(strlen(chan) + 1);
	if(!ircsock->chan) {
		ircsock_free(ircsock);
		return NULL;
	}
	strcpy(ircsock->chan, chan);
	
	return ircsock;
}

void ircsock_free(IRCSock *ircsock) {
	if(!ircsock)
		return;
	free(ircsock->host);
	free(ircsock->nick);
	free(ircsock->chan);
	free(ircsock);
}

