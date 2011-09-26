#include "ircsock.h"
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

/* Size of character buffers */
#define BCSIZE 4096
/* Size of line buffers */
#define BLSIZE 4096

IRCSock *ircsock_create(char *host, int port, char *nick, char *chan) { /*{{{*/
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

	ircsock->cbuf = cbuffer_create(BLSIZE);
	if(!ircsock->cbuf) {
		ircsock_free(ircsock);
		return NULL;
	}

	return ircsock;
} /*}}}*/

/* TODO: handle disconnecting */
void ircsock_free(IRCSock *ircsock) { /*{{{*/
	if(!ircsock)
		return;
	cbuffer_free(ircsock->cbuf);
	free(ircsock->host);
	free(ircsock->nick);
	free(ircsock->chan);
	free(ircsock);
} /*}}}*/

/* Function used to get address information from a domain and port
 *
 * Returns NULL on failure, prints error to stderr
 */
struct addrinfo *ircsock_lookupDomain(IRCSock *ircsock) { /*{{{*/
	struct addrinfo *result;
	char sport[BCSIZE];
	int error;

	if(ircsock->socket == -1) {
		fprintf(stderr, "Can't lookup domain: socket is nonexistant\n");
		return NULL;
	}

	snprintf(sport, BCSIZE, "%d", ircsock->port);
	error = getaddrinfo(ircsock->host, sport, NULL, &result);
	if(error) {
		fprintf(stderr, "Can't lookup domain: error!\n");
		return NULL;
	}

	return result;
} /*}}}*/

int ircsock_connect(IRCSock *ircsock) { /*{{{*/
	struct addrinfo *result;
	int error;

	ircsock->socket = socket(AF_INET, SOCK_STREAM, 0);
	if(ircsock->socket == -1)
		return 1;

	result = ircsock_lookupDomain(ircsock);
	if(!result)
		return 2;
	error = connect(ircsock->socket, result->ai_addr, result->ai_addrlen);
	if(error == -1)
		return 3;
	freeaddrinfo(result);

	return 0;
} /*}}}*/

ssize_t ircsock_read(IRCSock *ircsock) { /*{{{*/
	/* We must have enough space for the newline and null character */
	char buf[BCSIZE + 2];
	ssize_t ramount;
	char *str, *tok;

	ramount = read(ircsock->socket, buf, BCSIZE);
	if(ramount > 0) {
		buf[ramount] = '\n';
		buf[ramount + 1] = '\0';
		tok = strtok(buf, "\r\n");
		while(tok != NULL) {
			str = malloc(strlen(tok) + 1);
			if(!str) {
				fprintf(stderr, "Failed to allocate memory in ircsock_read!\n");
				return IRCSOCK_MERROR;
			}
			strcpy(str, tok);

			cbuffer_push(ircsock->cbuf, str);
			tok = strtok(NULL, "\r\n");
		}
	}
	return ramount;
} /*}}}*/

ssize_t ircsock_send(IRCSock *ircsock, char *str) { /*{{{*/
	ssize_t wamount = write(ircsock->socket, str, strlen(str));
	if(wamount < 0)
		return wamount;
	wamount += write(ircsock->socket, "\r\n", 2);
	return wamount;
} /*}}}*/

ssize_t ircsock_pmsg(IRCSock *ircsock, char *target, char *msg) { /*{{{*/
	char *buf = malloc(strlen(msg) + strlen("PRIVMSG ") + strlen(target) + 1);
	if(!buf) {
		fprintf(stderr, "Failed to malloc in irsock_pmsg!\n");
		return IRCSOCK_MERROR;
	}
	strcpy(buf, "PRIVMSG ");
	strcat(buf, target);
	strcat(buf, " ");
	strcat(buf, msg);
	return ircsock_send(ircsock, buf);
} /*}}}*/

int ircsock_join(IRCSock *ircsock) { /*{{{*/
	int foundPing = 0;
	/* these use 16 as it should cover needed space for 4 letter command, a
	 * space, and extra arguments besides nick/chan */
	char *nickc = malloc(16 + strlen(ircsock->nick));
	char *userc = malloc(16 + strlen(ircsock->nick) * 2);
	char *joinc = malloc(16 + strlen(ircsock->chan));
	char *str = NULL;

	if(!nickc || !userc || !joinc) {
		fprintf(stderr, "Failed malloc in ircsock_join\n");
		return IRCSOCK_MERROR;
	}

	strcpy(nickc, "NICK ");
	strcat(nickc, ircsock->nick);

	strcpy(userc, "USER ");
	strcat(userc, ircsock->nick);
	strcat(userc, " j j :");
	strcat(userc, ircsock->nick);

	strcpy(joinc, "JOIN ");
	strcat(joinc, ircsock->chan);

	ircsock_send(ircsock, nickc);
	ircsock_send(ircsock, userc);
	usleep(100000);

	while(!foundPing) {
		ircsock_read(ircsock);
		while((str = cbuffer_pop(ircsock->cbuf)) != NULL) {
			if(strstr(str, " 433 ")) {
				fprintf(stderr, "Nickname in use!\n");
				return 433;
			}
			if((str[0] == 'P') && (str[1] == 'I') &&
				(str[2] == 'N') && (str[3] == 'G') &&
				(str[4] == ' ') && (str[5] == ':')) {
				str[1] = 'O';
				ircsock_send(ircsock, str);
				free(str);

				foundPing = 1;
				break;
			}
			free(str);
		}
	}

	ircsock_send(ircsock, joinc);
	return 0;
} /*}}}*/

int ircsock_quit(IRCSock *ircsock) { /*{{{*/
	return ircsock_send(ircsock, "QUIT");
} /*}}}*/

