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
#include "greetings.h"
#include "defines.h"
#include "util.h"

// Container for all of the variables and configuration settings
BMap *varsMap = NULL;
BMap *confMap = NULL;

#define FUNCREGX(x, y) { #x, #y, 1, 0, NULL, &x }
#define FUNCTION(x)    FUNCREGX(x, ^x (.*)$)
FuncStruct functions[] = {
	FUNCTION(markov), FUNCTION(fish), FUNCTION(fishes), FUNCTION(dubstep),
	FUNCTION(sl), FUNCTION(declare), FUNCTION(set),
	FUNCTION(increment), { "++", "^\\+\\+ (.*)$", 1, 0, NULL, &increment },
	FUNCTION(decrement), { "--",     "^-- (.*)$", 1, 0, NULL, &decrement },
	FUNCTION(wave),
	{ "wave",    "^o/ (.*)$", 1, 0, NULL, &wave },
	{ "wave", "^\\\\o (.*)$", 1, 0, NULL, &wave },
	{ NULL, NULL, 0, 0, NULL, NULL },
};

int setupFunctions() {
	for(int i = 0; functions[i].name && functions[i].f; ++i) {
		functions[i].r = malloc(sizeof(regex_t));
		if(!functions[i].r) {
			fprintf(stderr, "Could not malloc space for %s regex!\n",
					functions[i].name);
			return 0;
		}
		//fprintf(stderr, "%s: \"%s\"\n", functions[i].name, functions[i].regx);
		// If we fail to compile the function regex, abort
		int fail = regcomp(functions[i].r, functions[i].regx, REG_EXTENDED);
		if(fail) {
			fprintf(stderr, "Could not compile %s regex!\n", functions[i].name);
			fprintf(stderr, "erromsg: %s\n", getRegError(fail, functions[i].r));
			return 0;
		}
	}
	return 1;
}

/* main runs through a loop getting input and sending output. We logFile
 * everything to a file name *lfname. See internals for commands recognized
 */
int main(int argc, char **argv) {
	if(argc > 1) {
		printf("Usage: %s", argv[0]);
		printf("Usually, this is run by conbot. If you want to run it manually,");
		printf(" you must type standard IRC broadcast messages\n");
		return 0;
	}

	char str[BSIZE], *tok, *tmsg, *cstart;
	char name[PBSIZE], hmask[PBSIZE], cname[PBSIZE], msg[BSIZE];

	regex_t pmsgRegex, joinRegex;
	regmatch_t mptr[16];

	// TODO: abuse macros? stringification/concatenation?
	const char *privmsgRegexExp =
		"^:([A-Za-z0-9_]*)!([-@~A-Za-z0-9_\\.]*) PRIVMSG ([#A-Za-z0-9_]*) :(.*)";
	const char *joinRegexExp =
		"^:([A-Za-z0-9_]*)!([-@~A-Za-z0-9_\\.]*) JOIN :([#A-Za-z0-9_]*)";

	// seed random number generator with current time
	srand(time(NULL));

	// If we fail to compile the PRIVMSG regex, abort
	int fail = regcomp(&pmsgRegex, privmsgRegexExp, REG_EXTENDED);
	if(fail) {
		fprintf(stderr, "Could not compile privmsg regex!\n");
		fprintf(stderr, "erromsg: %s\n", getRegError(fail, &pmsgRegex));
		return 1;
	}

	// If we fail to compile the JOIN regex, abort
	fail = regcomp(&joinRegex, joinRegexExp, REG_EXTENDED);
	if(fail) {
		fprintf(stderr, "Could not compile join regex!\n");
		fprintf(stderr, "erromsg: %s\n", getRegError(fail, &joinRegex));
		return 1;
	}

	// If we can't malloc/compile the function regex's, abort
	if(!setupFunctions())
		return 1;

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

	// try to read old variables in
	int count = bmap_read(varsMap, dumpFileName);
	if(count > 0) {
		send(owner, "Read in %d variables", count);
	}

	fflush(stdout);
	lflush();

	// main loop, go until we say we are done or parent is dead/closes us off
	int done = 0;
	while(!feof(stdin) && !done) {
		if(fgets(str, BSIZE - 1, stdin) == str) {
			// replace newline with null
			str[strlen(str) - 1] = '\0';

			// log all incoming data
			lprintf(" <- %s\n", str);

			fail = regexec(&pmsgRegex, str, pmsgRegex.re_nsub + 1, mptr, 0);
			// if a PRIVMSG was broadcast to us
			if(!fail) {
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
				char *msgp = msg;

				lprintf("PRIVMSG from %s to %s: %s\n", name, cname, msg);
				tok = strtok(tmsg, " ");

				int toUs = 0;
				// if this is targeted at us
				if(!strcmp(tok, cstart)) {
					// ignore it
					tok = strtok(NULL, " ");
					toUs = 1;
					if(strlen(msgp) <= strlen(cstart) + 1) {
						// then there is nothing to do a command on
						continue;
					}
					msgp += strlen(cstart) + 1;
				}
				if(!strcmp(cname, nick))
					toUs = 1;

				FunctionArgs fargs = {
					name, hmask, ((!strcmp(cname, nick)) ? name : chan),
					toUs, msgp, mptr, varsMap, confMap
				};

				int matched = 0;
				//fprintf(stderr, "Matching on \"%s\"\n", msgp);
				for(int i = 0; !matched && functions[i].f; ++i) {
					fail = regexec(functions[i].r,
							msgp, functions[i].r->re_nsub + 1, mptr, 0);
					if(!fail) {
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
			fail = regexec(&joinRegex, str, joinRegex.re_nsub + 1, mptr, 0);
			// if a JOIN was broadcast to us
			if(!fail) {
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
			if(!feof(stdin)) {
				// fgets failed, handle printing error message
				fprintf(stderr, "fgets failed in main jbot loop!\n");
				lprintf("fgets failed in main jbot loop!\n");
			}
		}
	}

	// try to dump variables for reload
	count = bmap_dump(varsMap, dumpFileName);
	if(count > 0) {
		send(owner, "Dumped %d variables", count);
	}

	fflush(stdout);
	lflush();

	// close stdin/stdout just to make sure conbot knows we stopped
	close(0);
	close(1);
	return 0;
}

