#include <stdio.h>
#include "ircsock.h"

#define SERVER ".slashnet.org"

IRCSock *ircSocket = NULL;

int main(int argc, char **argv) {
	char *prefix = "irc";
	if(argc > 1)
		prefix = argv[1];

	printf("Server: %s%s\n", prefix, SERVER);

	return 0;
}

