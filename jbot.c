#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <regex.h>
#include <unistd.h>
#include "ircsock.h"

#define SERVER ".slashnet.org"

IRCSock *ircSocket = NULL;

void *readLoop(void *args) {
	if(args != NULL)
		return NULL;
	while(1) {
		ircsock_read(ircSocket);
	}
}

char *getRegError(int errcode, regex_t *compiled) {
	size_t l = regerror(errcode, compiled, NULL, 0);
	char *buffer = malloc(l + 1);
	regerror(errcode, compiled, buffer, l);
	return buffer;
}

int main(int argc, char **argv) {
	char *nick = "jbotc", *chan = "#zebra";
	char *prefix = "irc", *str, *sname, *tok, *cstart, *msg;
	int port = 6667;
	pthread_t readThread;
	regex_t *pmsgRegex = malloc(sizeof(regex_t));
	regmatch_t mptr[4];
	int res, done = 0;

	res = regcomp(pmsgRegex,
			"^:([A-Za-z0-9_]*)!([-@~A-Za-z0-9_\\.]*) PRIVMSG (#[A-Za-z0-9_]*) :(.*)",
			REG_EXTENDED);
	if(res) {
		fprintf(stderr, "Could not compile regex!\n");
		fprintf(stderr, "erromsg: %s\n", getRegError(res, pmsgRegex));
		return 1;
	}

	if(argc > 1)
		prefix = argv[1];

	sname = malloc(strlen(SERVER) + strlen(prefix) + 1);
	strncpy(sname, prefix, strlen(prefix));
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
	pthread_create(&readThread, NULL, readLoop, NULL);

	printf("Main loop\n");
	while(!done) {
		while((str = cbuffer_pop(ircSocket->cbuf)) != NULL) {
			if((str[0] == 'P') && (str[1] == 'I') &&
				(str[2] == 'N') && (str[3] == 'G') &&
				(str[4] == ' ') && (str[5] == ':')) {
				str[1] = 'O';
				printf(" -- PING/%s\n", str);
				ircsock_send(ircSocket, str);
			} else if((res = regexec(pmsgRegex, str, pmsgRegex->re_nsub + 1, mptr, 0)) == 0) {
				printf("PRIVMSG recieved from %.*s@%.*s: %.*s\n",
						mptr[1].rm_eo - mptr[1].rm_so, str + mptr[1].rm_so,
						mptr[3].rm_eo - mptr[3].rm_so, str + mptr[3].rm_so,
						mptr[4].rm_eo - mptr[4].rm_so, str + mptr[4].rm_so);
				tok = strtok(str + mptr[4].rm_so, " ");
				if(!strcmp(tok, cstart)) {
					tok = strtok(NULL, " ");
					if(!strcmp(tok, "restart")) {
						done = 77;
					} else if(!strcmp(tok, "markov")) {
						tok = strtok(NULL, " ");
						if(tok == NULL) {
							msg = malloc(mptr[1].rm_eo - mptr[1].rm_so + 64);
							strncpy(msg, str + mptr[1].rm_so,
									mptr[1].rm_eo - mptr[1].rm_so);
							msg[mptr[1].rm_eo - mptr[1].rm_so] = '\0';
							strcat(msg, ": Usage: markov <word>");
							ircsock_pmsg(ircSocket, msg);
							free(msg);
						} else {
							msg = malloc(strlen(tok) +
									mptr[1].rm_eo - mptr[1].rm_so + 1);
							strncpy(msg, str + mptr[1].rm_so,
									mptr[1].rm_eo - mptr[1].rm_so);
							strcat(msg, tok);
							ircsock_pmsg(ircSocket, msg);
							free(msg);
						}
					} else {
						msg = malloc(mptr[1].rm_eo - mptr[1].rm_so +
								mptr[4].rm_eo - mptr[4].rm_so + 1);
						strncpy(msg, str + mptr[1].rm_so,
								mptr[1].rm_eo - mptr[1].rm_so);
						msg[mptr[1].rm_eo - mptr[1].rm_so] = '\0';
						strcat(msg, ": ");
						while(tok != NULL) {
							strcat(msg, tok);
							strcat(msg, " ");
							tok = strtok(NULL, " ");
						}
						ircsock_pmsg(ircSocket, msg);
						free(msg);
					}
				}
			} else {
				printf("%s\n", str);
			}
			free(str);
		}
		sleep(1);
	}

	if(done == 77)
		return 77;

	return 0;
}

