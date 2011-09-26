/* conbot's purpose is to handle connecting to an IRC network and joining a
 * room. It then stays connected, and only handles enough of the IRC protocol
 * to remain connected.
 *
 * To handle user input and output, it has transferLoop. Essentially, it sets
 * up read/write pipes to an invocation of the jbot progrom (jbot can be
 * anything that handles stdin and writes to stdout). If jbot dies, we restart
 * it. We send all non-connection related messages to it, and send its output
 * back to our IRCSock socket. See transferLoop for more details
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <regex.h>
#include <unistd.h>
#include "ircsock.h"
#include "cbuffer.h"

/* size of character buffers */
#define BSIZE 4096
/* Size of line buffers */
#define BLSIZE 1024

/* Name of server to connect to */
/* TODO: make not hard coded */
#define SERVER ".slashnet.org"

IRCSock *ircSocket = NULL;
CBuffer *cbuf = NULL;

/* name of binary to be invoked as subprocess */
char *jbotBinary = "jbot";
/* true if we can write to output pipe, false otherwise */
int childReady = 0;
/* oPipe is used in the pipe() call to create the pipe going out of conbot,
 * iPipe is used for the incoming pipe */
int oPipe[2];
int iPipe[2];
/* These are standard FILE * which we set up using fdopen() from the write end
 * of oPipe and the read end of iPipe */
FILE *ofpipe;
FILE *ifpipe;

/* readLoop is a function to be invoked as its own pthread. Since ircsock_read
 * blocks, this sits in its own thread an simply continuously dumps read input
 * into ircSocket->cbuf
 */
void *readLoop(void *args) { /*{{{*/
	if(args != NULL)
		return NULL;
	while(1) {
		ircsock_read(ircSocket);
	}
} /*}}}*/

/* Spawns subprocess and sets up ofpipe and ifpipe for reading and writing to
 * it. Sits in a loop in its own pthread until main process ends
 */
void *transferLoop(void *args) { /*{{{*/
	char str[BSIZE];
	pid_t cpid;
	if(args != NULL)
		return NULL;
	/* initialize pipes to invalid file descriptors */
	oPipe[0] = oPipe[1] = -1;
	iPipe[0] = iPipe[1] = -1;
	/* each loop through either opens up a new subproccess and reads/writes to
	 * it, or aborts early and tries again if there was a problem in setting up
	 * the required pipes/subprocess.
	 */
	while(1) {
		childReady = 0;
		/* If oPipe needs setup, attempt to create it. If we fail, print error
		 * message, try again in 15 seconds */
		if(((oPipe[0] == oPipe[1]) && (oPipe[0] == -1)) && pipe(oPipe)) { /*{{{*/
			fprintf(stderr, "Could not create out pipe...\n");
			fprintf(stderr, "Sleeping for 15 seconds and trying again...\n");
			sleep(15);
			continue;
		} /*}}}*/
		/* Follow the same process with iPipe */
		if(((iPipe[0] == iPipe[1]) && (iPipe[0] == -1)) && pipe(iPipe)) { /*{{{*/
			fprintf(stderr, "Could not create in pipe...\n");
			fprintf(stderr, "Sleeping for 15 seconds and trying again...\n");
			sleep(15);
			continue;
		} /*}}}*/

		printf("Forking subprocess...\n");
		/* Fork so that we can exec the subprocess, if we fail, try again in 15
		 * seconds */
		cpid = fork();
		if(cpid == -1) { /*{{{*/
			fprintf(stderr, "Could not fork!\n");
			fprintf(stderr, "Sleeping for 15 seconds and trying again...\n");
			sleep(15);
			continue;
		} /*}}}*/

		/* If we are the fork, setup stdin/stdout, exec jbot */
		if(cpid == 0) { /*{{{*/
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

			/* replace the fork image with the jbot binary */
			execv(jbotBinary, args);
			/* shouldn't ever get here */
			return NULL;
		} /*}}}*/

		/* If we are still a thread in the main process... */

		/* Close not-needed ends of {o,i}Pipe */
		close(oPipe[0]);
		close(iPipe[1]);
		/* Setup {o,i}fipe FILE *s for writing/reading respectively */
		ofpipe = fdopen(oPipe[1], "w");
		ifpipe = fdopen(iPipe[0], "r");

		printf("Child is ready, entering read loop...\n");
		childReady = 1;
		/* send all read data from subprocess to IRC server */
		while(!feof(ifpipe)) { /*{{{*/
			if(fgets(str, BSIZE - 1, ifpipe) == str) {
				str[strlen(str) - 1] = '\0';
				ircsock_send(ircSocket, str);
			}
		} /*}}}*/
		childReady = 0;

		printf("Child closed, restarting...\n");
		/* close remaining ends of {o,i}Pipe */
		close(oPipe[1]);
		close(iPipe[0]);
		/* reset pipes to invalid file descriptors */
		oPipe[0] = oPipe[1] = -1;
		iPipe[0] = iPipe[1] = -1;
		/* TODO: since we close() the actual pipe, do we still need to fclose()
		 * these that we opened with fdopen()? */
		ofpipe = NULL;
		ifpipe = NULL;
	}
} /*}}}*/

char *getRegError(int errcode, regex_t *compiled) {
	size_t l = regerror(errcode, compiled, NULL, 0);
	char *buffer = malloc(l + 1);
	regerror(errcode, compiled, buffer, l);
	return buffer;
}

int main(int argc, char **argv) {
	/* nick: the nick that this bot should identify as
	 * chan: the channel this bot should get on
	 * owner: the nick of the owner of this bot
	 * TODO: read in from config/default to these values
	 */
	char *nick = "jbotc", *chan = "#zebra", *owner = "jac";
	/* prefix: prefix to SERVER we should try to sign on to
	 * port: port on the server we should connect to
	 * TODO: read in from config/default to these values
	 */
	char *prefix = "irc";
	int port = 6667;

	char *str, *sname, *tok, *cstart;
	pthread_t readThread, transferThread;
	regex_t *pmsgRegex = malloc(sizeof(regex_t));
	regmatch_t mptr[16];
	int res, done = 0;

	/* If we recieve an argument, use it as the server prefix */
	if(argc > 1)
		prefix = argv[1];

	/* Compile the PRIVMSG regex, abort on failure. Since this should be
	 * tested, failure should NEVER happen */
	res = regcomp(pmsgRegex,
			"^:([A-Za-z0-9_]*)!([-@~A-Za-z0-9_\\.]*) PRIVMSG ([#A-Za-z0-9_]*) :(.*)",
			REG_EXTENDED);
	if(res) { /*{{{*/
		fprintf(stderr, "Could not compile regex!\n");
		fprintf(stderr, "erromsg: %s\n", getRegError(res, pmsgRegex));
		return 1;
	} /*}}}*/

	/* Create the output to jbot buffer, abort on failure */
	cbuf = cbuffer_create(BLSIZE);
	if(!cbuf) { /*{{{*/
		fprintf(stderr, "Could not create output buffer!\n");
		return 1;
	} /*}}}*/

	sname = malloc(strlen(SERVER) + strlen(prefix) + 1);
	if(!sname) {
		fprintf(stderr, "Could not malloc space for sname!\n");
		return 1;
	}
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
	if(ircsock_connect(ircSocket) != 0) {
		fprintf(stderr, "Couldn't connect!\n");
		return 1;
	}

	printf("Joining %s as %s...\n", chan, nick);
	if(ircsock_join(ircSocket) != 0) {
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

