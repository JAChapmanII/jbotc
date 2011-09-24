#include <stdio.h>
#include "ircsock.h"

char *SERVER = ".slashnet.org";

int main(int argc, char **argv) {
	char *serverPrefx = "irc";
	if(argc > 1)
		serverPrefx = argv[1];

	printf("Server: %s%s\n", serverPrefx, SERVER);

	return 0;
}

