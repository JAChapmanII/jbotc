#include "ircsock.h"
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

IRCSock *ircsock_create(char *host, int port, char *nick, char *chan) {
	IRCSock *ircsock = malloc(sizeof(IRCSock));
	if(!ircsock)
		return NULL;

	ircsock->host = NULL;
	ircsock->nick = NULL;
	ircsock->chan = NULL;
	ircsock->cbuf = NULL;

	ircsock->host = malloc(strlen(host) + 1);
	if(!ircsock->host) {
		ircsock_free(ircsock);
		return NULL;
	}
	strcpy(ircsock->host, host);

	ircsock->port = port;
	ircsock->domain = -1;

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

	ircsock->socket = -1;

	ircsock->cbuf = cbuffer_create(4096);

	return ircsock;
}

void ircsock_free(IRCSock *ircsock) {
	if(!ircsock)
		return;
	cbuffer_free(ircsock->cbuf);
	free(ircsock->host);
	free(ircsock->nick);
	free(ircsock->chan);
	free(ircsock);
}

struct addrinfo *ircsock_lookupDomain(IRCSock *ircsock) {
	struct addrinfo *result;
	char sport[64];
	int error;

	if(ircsock->socket == -1) {
		fprintf(stderr, "Can't lookup domain: socket is nonexistant\n");
		return NULL;
	}

	snprintf(sport, 64, "%d", ircsock->port);
	error = getaddrinfo(ircsock->host, sport, NULL, &result);
	if(error) {
		fprintf(stderr, "Can't lookup domain: error!\n");
		return NULL;
	}

	return result;
}

int ircsock_connect(IRCSock *ircsock) {
	struct addrinfo *result;
	int error;

	ircsock->socket = socket(AF_INET, SOCK_STREAM, 0);
	if(ircsock->socket == -1)
		return 0;

	result = ircsock_lookupDomain(ircsock);
	if(!result)
		return 0;
	error = connect(ircsock->socket, result->ai_addr, result->ai_addrlen);
	if(error == -1)
		return 0;
	freeaddrinfo(result);

	return 1;
}

void ircsock_read(IRCSock *ircsock) {
	char buf[4096];
	ssize_t ramount;
	char *str, *tok;

	ramount = read(ircsock->socket, buf, 4096);
	if(ramount > 0) {
		buf[ramount] = '\n';
		buf[ramount + 1] = '\0';
		tok = strtok(buf, "\r\n");
		while(tok != NULL) {
			str = malloc(strlen(tok) + 1);
			strcpy(str, tok);

			cbuffer_push(ircsock->cbuf, str);
			tok = strtok(NULL, "\r\n");
		}
	}
}

void ircsock_send(IRCSock *ircsock, char *str) {
	write(ircsock->socket, str, strlen(str));
	write(ircsock->socket, "\r\n", 2);
}

