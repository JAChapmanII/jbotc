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

#include "functions.h"
#include "util.h"

#define BSIZE 4096
#define PBSIZE 256

// Container for all of the variables.
BMap *variableMap = NULL;

/* markov will eventually print markov chains generated from previous input. */
void markov(const char *name, char *tok) { // {{{
	tok = strtok(NULL, " ");
	if(tok == NULL) {
		// if we didn't recieve an argument, print usage
		send(chan, "%s: Usage: markov <word>", name);
	} else {
		// we recieved an argument, print it back to them for now.
		// TODO: implement properly
		send(chan, "%s: %s", name, tok);
	}
} // }}}

/* Declares variables to remember things. */
void declareVariable(const char *name, char *tok) {
	BMap_Node *tmpn = NULL;

	tok = strtok(NULL, " ");
	tmpn = bmap_find(variableMap, tok);
	if(tmpn == NULL) {
		if(bmap_size(variableMap) >= 256) {
			send(chan, "%s: 256 variables exist already, sorry!", name);
		} else {
			bmap_add(variableMap, tok, "0");
			send(chan, "%s: set \"%s\" to 0", name, tok);
		}
	} else {
		send(chan, "%s: \"%s\" is %s", name, tok, tmpn->val);
	}
}

/* Set a variable to remember things. */
void setVariable(const char *name, char *tok) {
	BMap_Node *tmpn = NULL;
	char *tmpsp;

	tok = strtok(NULL, " ");
	tmpsp = tok;
	tok = strtok(NULL, " ");
	if(!tok || !tmpsp) {
		send(chan, "%s: You must specify a variable and value", name);
	} else {
		bmap_add(variableMap, tmpsp, tok);
		tmpn = bmap_find(variableMap, tmpsp);
		if(!tmpn) {
			send(chan, "%s: \"%s\" was not found!?", name, tmpsp);
		} else {
			send(chan, "%s: \"%s\" is %s", name, tmpsp, tmpn->val);
		}
	}
}

/* Increment a variable and create it if it doesn't exist. */
void incrementVariable(const char *name, char *tok) {
	BMap_Node *tmpn = NULL;

	tok = strtok(NULL, " ");
	tmpn = bmap_find(variableMap, tok);
	if(tmpn == NULL) {
		if(bmap_size(variableMap) >= 256) {
			send(chan, "%s: 256 variables exist already, sorry!", name);
		} else {
			bmap_add(variableMap, tok, "0");
			send(chan, "%s: set \"%s\" to 0", name, tok);
		}
	} else {
		int tmp;
		char tmps[PBSIZE];

		tmp = atoi(tmpn->val);
		++tmp;
		snprintf(tmps, PBSIZE, "%d", tmp);
		bmap_add(variableMap, tok, tmps);
		send(chan, "%s: \"%s\" is %d", name, tok, tmp);
	}
}

/* Decrement a variable and create it if it doesn't exist. */
void decrementVariable(const char *name, char *tok) {
	BMap_Node *tmpn = NULL;

	tok = strtok(NULL, " ");
	tmpn = bmap_find(variableMap, tok);
	if(tmpn == NULL) {
		if(bmap_size(variableMap) >= 256) {
			send(chan, "%s: 256 variables exist already, sorry!", name);
		} else {
			bmap_add(variableMap, tok, "0");
			send(chan, "%s: set \"%s\" to 0", name, tok);
		}
	} else {
		int tmp;
		char tmps[PBSIZE];

		tmp = atoi(tmpn->val);
		tmp--;
		snprintf(tmps, PBSIZE, "%d", tmp);
		bmap_add(variableMap, tok, tmps);
		send(chan, "%s: \"%s\" is %d", name, tok, tmp);
	}
}
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
	if((variableMap = bmap_create()) == NULL) {
		fprintf(stderr, "Could not create contstant map!\n");
		return 1;
	}

	// If we fail to open the logFile in append mode, abort
	if(!initLogFile())
		return 1;

	// print separator to logFile
	fprintf(logFile, "------------------------------------\n");

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
	fflush(logFile);

	// main loop, go until we say we are done or parent is dead/closes us off
	while(!feof(stdin) && !done) {
		if(fgets(str, BSIZE - 1, stdin) == str) {
			// replace newline with null
			str[strlen(str) - 1] = '\0';

			// logFile input
			fprintf(logFile, " <- %s\n", str);
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

				fprintf(logFile, "PRIVMSG from %s to %s: %s\n", name, cname, msg);
				tok = strtok(tmsg, " ");

				toUs = 0;
				// if this is targeted at us
				if(!strcmp(tok, cstart)) {
					// ignore it
					tok = strtok(NULL, " ");
					toUs = 1;
				}

				// reload stops this instance, parent conbot starts new one
				if(!strcmp(tok, "reload") && !strcmp(name, owner) && toUs) {
					fprintf(logFile, "Got message to restart...\n");
					done = 1;

				// markov prints markov chains generated from previous input
				} else if(!strcmp(tok, "markov")) {
					markov(name, tok);

				// CodeBlock wants a fish...
				} else if(!strcmp(tok, "fish")) {
					send(chan, "%s: %s", name, ((rand() % 2) ? "><>" : "<><"));

				// CodeBlock wants multiple species of fish
				} else if(!strcmp(tok, "fishes")) {
					send(chan, "%s: ><> <>< <><   ><> ><>", name);

				// WUB WUB WUB WUB WUB
				} else if(!strcmp(tok, "dubstep")) {
					send(chan, "%s: WUB WUB WUB", name);

				// declaring a variable
				} else if(!strcmp(tok, "declare")) {
					declareVariable(name, tok);

				// setting a variable
				} else if(!strcmp(tok, "set")) {
					setVariable(name, tok);

				// incrementing a variable (or declaring it)
				} else if(!strcmp(tok, "inc") || !strcmp(tok, "++")) {
					incrementVariable(name, tok);

				// decrementing a variable (or declaring it)
				} else if(!strcmp(tok, "dec") || !strcmp(tok, "--")) {
					decrementVariable(name, tok);

				// token after cstart does not match command
				} else {
					if(toUs) {
						// msg ends with question mark, guess an answer
						if((strlen(msg) > 0) && (msg[strlen(msg) - 1] == '?')) {
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
			fflush(logFile);
		} else {
			// fgets failed, handle printing error message
			fprintf(stderr, "fgets failed in main jbot loop!\n");
			fprintf(logFile, "fgets failed in main jbot loop!\n");
		}
	}

	// close stdin/stdout just to make sure conbot knows we stopped
	close(0);
	close(1);
	return 0;
}

