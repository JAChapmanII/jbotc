#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <regex.h>
#include <unistd.h>
#include "ircsock.h"

#define BSIZE 4096
#define SERVER ".slashnet.org"

IRCSock *ircSocket = NULL;

void *readLoop(void *args) {
	if(args != NULL)
		return NULL;
	while(1) {
		ircsock_read(ircSocket);
	}
}

void *transferLoop(void *args) {
	if(args != NULL)
		return NULL;
	while(1) {
		/*
		while((str = cbuffer_pop(pSocket->cbuf)) != NULL) {
			ircsock_send(ircSocket, str);
			free(str);
		}
		*/
		usleep(5000);
	}
}

char *getRegError(int errcode, regex_t *compiled) {
	size_t l = regerror(errcode, compiled, NULL, 0);
	char *buffer = malloc(l + 1);
	regerror(errcode, compiled, buffer, l);
	return buffer;
}

int main(int argc, char **argv) {
	char *nick = "jbotc", *chan = "#zebra", *owner = "jac";
	char *prefix = "irc", *str, *sname, *tok, *cstart;
	int port = 6667;
	pthread_t readThread, transferThread;
	regex_t *pmsgRegex = malloc(sizeof(regex_t));
	regmatch_t mptr[16];
	int res, done = 0;

	if(argc > 1)
		prefix = argv[1];

	res = regcomp(pmsgRegex,
			"^:([A-Za-z0-9_]*)!([-@~A-Za-z0-9_\\.]*) PRIVMSG (#[A-Za-z0-9_]*) :(.*)",
			REG_EXTENDED);
	if(res) {
		fprintf(stderr, "Could not compile regex!\n");
		fprintf(stderr, "erromsg: %s\n", getRegError(res, pmsgRegex));
		return 1;
	}

	sname = malloc(strlen(SERVER) + strlen(prefix) + 1);
	strncpy(sname, prefix, strlen(prefix));
	sname[strlen(prefix)] = '\0';
	strcat(sname, SERVER);
	printf("Server: %s\n", sname);

	cstart = malloc(strlen(nick) + 2);
	strcpy(cstart, nick);
	strcat(cstart, ":");

	printf("Creating ircSocket...\n");
	ircSocket = ircsock_create(sname, port, nick, chan);
	if(!ircSocket) {
		fprintf(stderr, "Could not create ircSocket!\n");
		return 1;
	}
	printf("Connecting...\n");
	if(!ircsock_connect(ircSocket)) {
		fprintf(stderr, "Couldn't connect!\n");
		return 1;
	}

	printf("Joining %s as %s...\n", chan, nick);
	if(!ircsock_join(ircSocket)) {
		fprintf(stderr, "Couldn't join room!\n");
		return 1;
	}

	printf("Creating read thread...\n");
	if(pthread_create(&readThread, NULL, readLoop, NULL) != 0) {
		fprintf(stderr, "Could not create readThread!\n");
		ircsock_quit(ircSocket);
		return 1;
	}

	printf("Creating transfer thread...\n");
	if(pthread_create(&transferThread, NULL, transferLoop, NULL) != 0) {
		fprintf(stderr, "Could not create readThread!\n");
		ircsock_quit(ircSocket);
		return 1;
	}

	printf("Entering output loop\n");
	while(!done) {
		while((str = cbuffer_pop(ircSocket->cbuf)) != NULL) {
			if((str[0] == 'P') && (str[1] == 'I') &&
				(str[2] == 'N') && (str[3] == 'G') &&
				(str[4] == ' ') && (str[5] == ':')) {
				str[1] = 'O';
				printf(" -- PING/%s\n", str);
				ircsock_send(ircSocket, str);
			} else {
				res = regexec(pmsgRegex, str, pmsgRegex->re_nsub + 1, mptr, 0);
				if(res == 0) {
					tok = strtok(str + mptr[4].rm_so, " ");
					if(!strcmp(tok, cstart)) {
						tok = strtok(NULL, " ");
						if(!strcmp(tok, "restart")) {
							done = 77;
						}
					}
				}
			}
			free(str);
		}
		usleep(5000);
	}

	ircsock_quit(ircSocket);

	if(done == 77)
		return 77;
	return 0;
}

