#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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

int main(int argc, char **argv) {
	char *nick = "jbotc", *chan = "#zebra";
	char *prefix = "irc", *str, *sname;
	int port = 6667;
	pthread_t readThread;

	if(argc > 1)
		prefix = argv[1];

	sname = malloc(strlen(SERVER) + strlen(prefix) + 1);
	strncpy(sname, prefix, strlen(prefix));
	strcat(sname, SERVER);
	printf("Server: %s\n", sname);

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
	ircsock_join(ircSocket);

	printf("Creating read thread...\n");
	pthread_create(&readThread, NULL, readLoop, NULL);

	printf("Main loop\n");
	while(1) {
		while((str = cbuffer_pop(ircSocket->cbuf)) != NULL) {
			printf("%s\n", str);
			if((str[0] == 'P') && (str[1] == 'I') &&
				(str[2] == 'N') && (str[3] == 'G') &&
				(str[4] == ' ') && (str[5] == ':')) {
				printf(" <- %s\n", str);
				str[1] = 'O';
				printf(" -> %s\n", str);
				ircsock_send(ircSocket, str);
				free(str);
			}
		}
	}

	return 0;
}

