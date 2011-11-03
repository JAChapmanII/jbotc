/* jbot is the program doing the major parsing of input and sending output
 * back to users. It is run by conbot, which sets up stdin and stdout to go to
 * pipes from which conbot can read and write to.
 *
 * Since jbotc reads from stdin and writes to stdout, it can be run in a
 * terminal and fed made up input for testing purposes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <time.h>

#include "functions.h"
#include "defines.h"

// Container for all of the variables and configuration settings
BMap *varsMap = NULL;
BMap *confMap = NULL;

#define FUNCTION(x) { #x, &x }
FuncStruct functions[] = {
	FUNCTION(markov), FUNCTION(fish), FUNCTION(fishes), FUNCTION(dubstep),
	FUNCTION(sl), FUNCTION(declare), FUNCTION(set),
	FUNCTION(increment), { "++", &increment },
	FUNCTION(decrement), { "--", &decrement },
	FUNCTION(wave), { "o/", &wave }, { "\\o", &wave },
	{ NULL, NULL },
};

/* main runs through a loop getting input and sending output. We logFile
 * everything to a file name *lfname. See internals for commands recognized
 */
int main(int argc, char **argv) {
	char str[BSIZE], *tok, *tmsg, *cstart;
	char name[PBSIZE], hmask[PBSIZE], cname[PBSIZE], msg[BSIZE];

	regex_t pmsgRegex, joinRegex;
	regmatch_t mptr[16];
	int res, done = 0, toUs;

	// TODO: abuse macros? stringification/concatenation?
	const char *privmsgRegexExp =
		"^:([A-Za-z0-9_]*)!([-@~A-Za-z0-9_\\.]*) PRIVMSG ([#A-Za-z0-9_]*) :(.*)";
	const char *joinRegexExp =
		"^:([A-Za-z0-9_]*)!([-@~A-Za-z0-9_\\.]*) JOIN :([#A-Za-z0-9_]*)";

	// seed random number generator with current time
	srand(time(NULL));

	// If we fail to compile the PRIVMSG regex, abort
	res = regcomp(&pmsgRegex, privmsgRegexExp, REG_EXTENDED);
	if(res) {
		fprintf(stderr, "Could not compile privmsg regex!\n");
		fprintf(stderr, "erromsg: %s\n", getRegError(res, &pmsgRegex));
		return 1;
	}

	// If we fail to compile the JOIN regex, abort
	res = regcomp(&joinRegex, joinRegexExp, REG_EXTENDED);
	if(res) {
		fprintf(stderr, "Could not compile join regex!\n");
		fprintf(stderr, "erromsg: %s\n", getRegError(res, &joinRegex));
		return 1;
	}

	// If we fail to create a basic map, abort
	if((varsMap = bmap_create()) == NULL) {
		fprintf(stderr, "Could not create contstant map!\n");
		return 1;
	}
	// If we fail to create a basic map, abort
	if((confMap = bmap_create()) == NULL) {
		fprintf(stderr, "Could not create configuration map!\n");
		return 1;
	}

	// If we fail to open the logFile in append mode, abort
	if(!initLogFile())
		return 1;

	// print separator to logFile
	lprintf("------------------------------------\n");

	// Setup the command start string, abort on failure
	cstart = malloc(strlen(nick) + 2);
	if(!cstart) {
		fprintf(stderr, "Could not malloc space for command start string!\n");
		return 1;
	}
	strcpy(cstart, nick);
	strcat(cstart, ":");

	send(owner, "%s", obtainGreeting());

	fflush(stdout);
	lflush();

	// main loop, go until we say we are done or parent is dead/closes us off
	while(!feof(stdin) && !done) {
		if(fgets(str, BSIZE - 1, stdin) == str) {
			// replace newline with null
			str[strlen(str) - 1] = '\0';

			// log all incoming data
			lprintf(" <- %s\n", str);

			res = regexec(&pmsgRegex, str, pmsgRegex.re_nsub + 1, mptr, 0);
			// if a PRIVMSG was broadcast to us
			if(res == 0) {
				// copy nick of sender into name
				strncpy(name, str + mptr[1].rm_so, mptr[1].rm_eo - mptr[1].rm_so);
				name[mptr[1].rm_eo - mptr[1].rm_so] = '\0';

				// copy host mask of sender into hmask
				strncpy(hmask, str + mptr[2].rm_so, mptr[2].rm_eo - mptr[2].rm_so);
				hmask[mptr[2].rm_eo - mptr[2].rm_so] = '\0';

				// copy target of PRIVMSG into cname
				strncpy(cname, str + mptr[3].rm_so, mptr[3].rm_eo - mptr[3].rm_so);
				cname[mptr[3].rm_eo - mptr[3].rm_so] = '\0';

				// copy message part into msg
				strcpy(msg, str + mptr[4].rm_so);
				// setup tmsg to point to the msg in str
				tmsg = str + mptr[4].rm_so;

				lprintf("PRIVMSG from %s to %s: %s\n", name, cname, msg);
				tok = strtok(tmsg, " ");

				toUs = 0;
				// if this is targeted at us
				if(!strcmp(tok, cstart)) {
					// ignore it
					tok = strtok(NULL, " ");
					toUs = 1;
				}
				if(!strcmp(cname ,nick))
					toUs = 1;

				FunctionArgs fargs = {
					name, hmask, ((!strcmp(cname, nick)) ? name : chan), tok,
					varsMap, confMap
				};

				int matched = 0;
				for(int i = 0; functions[i].name && functions[i].f; ++i) {
					if(!strcmp(tok, functions[i].name)) {
						matched = 1;
						functions[i].f(&fargs);
					}
				}

				// none of the standard functions matched
				if(!matched) {
					// reload stops this instance, parent conbot starts new one
					if(!strcmp(tok, "reload") && !strcmp(name, owner) && toUs) {
						lprintf("Got message to restart...\n");
						done = 1;
					// token after cstart does not match any command
					} else {
						// msg ends with question mark, guess an answer
						if(toUs && (strlen(msg) > 0) && (msg[strlen(msg) - 1] == '?')) {
							send(chan, "%s: %s", name, ((rand() % 2) ? "Yes" : "No"));
						}
					}
				}
			}
			res = regexec(&joinRegex, str, joinRegex.re_nsub + 1, mptr, 0);
			// if a JOIN was broadcast to us
			if(res == 0) {
				// copy nick of sender into name
				strncpy(name, str + mptr[1].rm_so, mptr[1].rm_eo - mptr[1].rm_so);
				name[mptr[1].rm_eo - mptr[1].rm_so] = '\0';

				// copy host mask of sender into hmask
				strncpy(hmask, str + mptr[2].rm_so, mptr[2].rm_eo - mptr[2].rm_so);
				hmask[mptr[2].rm_eo - mptr[2].rm_so] = '\0';

				// copy target of PRIVMSG into cname
				strncpy(cname, str + mptr[3].rm_so, mptr[3].rm_eo - mptr[3].rm_so);
				cname[mptr[3].rm_eo - mptr[3].rm_so] = '\0';

				/*
				if(strcmp(name, "ajanata"))
					send(chan, "%s: %s", name, obtainGreeting());
					*/
			}
			// flush everything so output goes out immediately
			fflush(stdout);
			lflush();
		} else {
			// fgets failed, handle printing error message
			fprintf(stderr, "fgets failed in main jbot loop!\n");
			lprintf("fgets failed in main jbot loop!\n");
		}
	}

	// close stdin/stdout just to make sure conbot knows we stopped
	close(0);
	close(1);
	return 0;
}

