#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <regex.h>
#include <unistd.h>
#include "ircsock.h"
#include "cbuffer.h"

#define BSIZE 4096
#define SERVER ".slashnet.org"

IRCSock *ircSocket = NULL;
CBuffer *cbuf = NULL;

char *jbotBinary = "jbot";
int childReady = 0;
int oPipe[2];
FILE *ofpipe;
int iPipe[2];
FILE *ifpipe;

void *readLoop(void *args) {
	if(args != NULL)
		return NULL;
	while(1) {
		ircsock_read(ircSocket);
	}
}

void *transferLoop(void *args) {
	char str[BSIZE];
	pid_t cpid;
	if(args != NULL)
		return NULL;
	oPipe[0] = oPipe[1] = -1;
	iPipe[0] = iPipe[1] = -1;
	while(1) {
		childReady = 0;
		if(((oPipe[0] == oPipe[1]) && (oPipe[0] == -1)) && pipe(oPipe)) {
			fprintf(stderr, "Could not create out pipe...\n");
			fprintf(stderr, "Sleeping for 15 seconds and trying again...\n");
			sleep(15);
			continue;
		}
		if(((iPipe[0] == iPipe[1]) && (iPipe[0] == -1)) && pipe(iPipe)) {
			fprintf(stderr, "Could not create in pipe...\n");
			fprintf(stderr, "Sleeping for 15 seconds and trying again...\n");
			sleep(15);
			continue;
		}
		printf("Forking subprocess...\n");
		cpid = fork();
		if(cpid == -1) {
			fprintf(stderr, "Could not fork!\n");
			fprintf(stderr, "Sleeping for 15 seconds and trying again...\n");
			sleep(15);
			continue;
		}
		if(cpid == 0) {
			/* prepare subprocess input */
			close(0);
			dup(oPipe[0]);
			close(oPipe[0]);
			close(oPipe[1]);

			/* prepare subprocess output */
			close(1);
			dup(iPipe[1]);
			close(iPipe[0]);
			close(iPipe[1]);

			execv(jbotBinary, args);
			return NULL;
		}
		printf("Forked...\n");
		close(oPipe[0]);
		ofpipe = fdopen(oPipe[1], "w");
		close(iPipe[1]);
		ifpipe = fdopen(iPipe[0], "r");
		childReady = 1;
		while(!feof(ifpipe)) {
			if(fgets(str, BSIZE - 1, ifpipe) == str) {
				str[strlen(str) - 1] = '\0';
				ircsock_send(ircSocket, str);
			}
		}
		printf("Child closed, restarting...\n");
		childReady = 0;
		close(oPipe[1]);
		close(iPipe[0]);
		oPipe[0] = oPipe[1] = -1;
		iPipe[0] = iPipe[1] = -1;
		ofpipe = NULL;
		ifpipe = NULL;
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
			"^:([A-Za-z0-9_]*)!([-@~A-Za-z0-9_\\.]*) PRIVMSG ([#A-Za-z0-9_]*) :(.*)",
			REG_EXTENDED);
	if(res) {
		fprintf(stderr, "Could not compile regex!\n");
		fprintf(stderr, "erromsg: %s\n", getRegError(res, pmsgRegex));
		return 1;
	}

	cbuf = cbuffer_create(1024);
	if(!cbuf) {
		fprintf(stderr, "Could not create output buffer!\n");
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
				tok = malloc(strlen(str) + 1);
				strcpy(tok, str);
				cbuffer_push(cbuf, tok);

				res = regexec(pmsgRegex, str, pmsgRegex->re_nsub + 1, mptr, 0);
				if(res == 0) {
					tok = strtok(str + mptr[4].rm_so, " ");
					if(!strcmp(tok, cstart)) {
						tok = strtok(NULL, " ");
						if(!strcmp(tok, "restart"))
							done = 77;
					}
				}

				if(childReady) {
					while((tok = cbuffer_pop(cbuf)) != NULL) {
						fprintf(ofpipe, "%s\n", tok);
						free(tok);
					}
					fflush(ofpipe);
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

