/* jbot is the program doing the major parsing of input and sending output
 * back to users. It is run by conbot, which sets up stdin and stdout to go to
 * pipes from which conbot can read and write to.
 *
 * Since jbotc reads from stdin and writes to stdout, it can be run in a
 * terminal and fed made up input for testing purposes
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <time.h>

#include "bmap.h"

#define BSIZE 4096
#define PBSIZE 256

/* Returns a pointer to the string representation of a regular expression
 * error. Since this should be tested, this function should never be called in
 * production.
 */
char *getRegError(int errcode, regex_t *compiled) { /*{{{*/
	size_t l = regerror(errcode, compiled, NULL, 0);
	char *buffer = malloc(l + 1);
	if(!buffer)
		return "Couldn't malloc space for regex errror!";
	regerror(errcode, compiled, buffer, l);
	return buffer;
} /*}}}*/

FILE *logFile = NULL;
/* Small wrapper to allow printing to a logFile and stdout at the same time {{{ */
void send(char *target, char *format, ...) {
	va_list args;
	char buf[BSIZE];

	va_start(args, format);
	vsnprintf(buf, BSIZE, format, args);

	printf("PRIVMSG %s :%s\n", target, buf);
	fprintf(logFile, " -> @%s :%s\n", target, buf);

	va_end(args);
} /*}}}*/

/* main runs through a loop getting input and sending output. We logFile
 * everything to a file name *lfname. See internals for commands recognized
 */
int main(int argc, char **argv) {
	/* nick: the nick of the bot we are running as
	 * chan: the channel we should be in and handling input from
	 * owner: the nick of our owner, may be used for special commands/output
	 * lfname: the name of the logFile file we should write to
	 * TODO: have these be read in or default to these constants
	 */
	char *nick = "jbotc", *chan = "#zebra", *owner = "jac", *lfname = "jbot.log";

	char str[BSIZE], *tok, *tmsg, *cstart;
	char name[PBSIZE], hmask[PBSIZE], cname[PBSIZE], msg[BSIZE];

	bMap *constantMap = NULL;

	regex_t pmsgRegex;
	regmatch_t mptr[16];
	int res, done = 0, r;

	/* seed random number generator with current time */
	srand(time(NULL));

	/* If we fail to compile the PRIVMSG regex, abort */
	res = regcomp(&pmsgRegex,
			"^:([A-Za-z0-9_]*)!([-@~A-Za-z0-9_\\.]*) PRIVMSG ([#A-Za-z0-9_]*) :(.*)",
			REG_EXTENDED);
	if(res) {
		fprintf(stderr, "Could not compile regex!\n");
		fprintf(stderr, "erromsg: %s\n", getRegError(res, &pmsgRegex));
		return 1;
	}

	/* If we fail to open the logFile in append mode, abort */
	logFile = fopen(lfname, "a");
	if(!logFile) {
		fprintf(stderr, "Could not open logFile file!\n");
		return 1;
	}

	/* print separator to logFile */
	fprintf(logFile, "------------------------------------\n");

	/* Setup the command start string, abort on failure */
	cstart = malloc(strlen(nick) + 2);
	if(!cstart) {
		fprintf(stderr, "Could not malloc space for command start string!\n");
		return 1;
	}
	strcpy(cstart, nick);
	strcat(cstart, ":");

	/* main loop, go until we say we are done or parent is dead/closes us off */
	while(!feof(stdin) && !done) {
		if(fgets(str, BSIZE - 1, stdin) == str) {
			/* replace newline with null */
			str[strlen(str) - 1] = '\0';

			/* logFile input */
			fprintf(logFile, " <- %s\n", str);
			res = regexec(&pmsgRegex, str, pmsgRegex.re_nsub + 1, mptr, 0);
			/* if a PRIVMSG was broadcast to us */
			if(res == 0) {
				/* copy nick of sender into name */
				strncpy(name, str + mptr[1].rm_so, mptr[1].rm_eo - mptr[1].rm_so);
				name[mptr[1].rm_eo - mptr[1].rm_so] = '\0';

				/* copy host mask of sender into hmask */
				strncpy(hmask, str + mptr[2].rm_so, mptr[2].rm_eo - mptr[2].rm_so);
				hmask[mptr[2].rm_eo - mptr[2].rm_so] = '\0';

				/* copy target of PRIVMSG into cname */
				strncpy(cname, str + mptr[3].rm_so, mptr[3].rm_eo - mptr[3].rm_so);
				cname[mptr[3].rm_eo - mptr[3].rm_so] = '\0';

				/* copy message part into msg */
				strcpy(msg, str + mptr[4].rm_so);
				/* setup tmsg to point to the msg in str */
				tmsg = str + mptr[4].rm_so;

				fprintf(logFile, "PRIVMSG %s :PRIVMSG recieved from %s@%s: %s\n",
						owner, name, cname, msg);
				tok = strtok(tmsg, " ");
				/* if this is targeted at us */
				if(!strcmp(tok, cstart)) {
					tok = strtok(NULL, " ");
					/* reload stops this instance, parent conbot starts new one */
					if(!strcmp(tok, "reload")) {
						fprintf(logFile, "Got message to restart...\n");
						done = 1;
					/* markov will eventually print markov chains generated from
					 * previous input. TODO: implement */
					} else if(!strcmp(tok, "markov")) {
						tok = strtok(NULL, " ");
						if(tok == NULL) {
							/* if we didn't recieve an argument, print usage */
							send(chan, "%s: Usage: markov <word>", name);
						} else {
							/* we recieved an argument, print it back to them for
							 * now. TODO: implement properly */
							send(chan, "%s: %s", name, tok);
						}
					/* CodeBlock wants a fish... */
					} else if(!strcmp(tok, "fish")) {
						r = rand();
						send(chan, "%s: %s", name, ((r % 2) ? "><>" : "<><"));
					/* CodeBlock wants multiple species of fish */
					} else if(!strcmp(tok, "fishes")) {
						send(chan, "%s: ><> <>< <><   ><> ><>", name);
					/* token after cstart does not match command */
					} else {
						/* msg ends with question mark, guess an answer */
						if((strlen(msg) > 0) && (msg[strlen(msg) - 1] == '?')) {
							r = rand();
							send(chan, "%s: %s", name, ((r % 2) ? "Yes" : "No"));
						}
					}
				}
			}
			/* flush everything so output goes out immediately */
			fflush(stdout);
			fflush(logFile);
		/* fgets failed, handle printing error message */
		} else {
			fprintf(stderr, "fgets failed in main jbot loop!\n");
			fprintf(logFile, "fgets failed in main jbot loop!\n");
		}
	}

	/* close stdin/stdout just to make sure conbot knows we stopped */
	close(0);
	close(1);
	return 0;
}

