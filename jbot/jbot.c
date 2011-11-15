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
#include "rlist.h"
#include "markov.h"

// Container for all of the variables and configuration settings
BMap *varsMap = NULL;
BMap *confMap = NULL;

// Container for regex based "is" function
RList *regexList = NULL;

/* Add a regex to the regexList for later matching */
void addRegex(FunctionArgs *fa) { // {{{
	if(!fa->toUs)
		return;
	if(rlist_size(regexList) >= 256) {
		send(fa->target, "%s: We already have 256 \"is\" clauses!", fa->name);
		return;
	}
	fa->matchedOn[fa->matches[1].rm_eo] = '\0';
	fa->matchedOn[fa->matches[2].rm_eo] = '\0';
	char *msg = rlist_add(regexList,
				fa->matchedOn + fa->matches[1].rm_so,
				fa->matchedOn + fa->matches[2].rm_so);
	send(fa->target, "%s: %s", fa->name, (msg ? msg : "Okay!"));
} // }}}

/* Remove a regex from regexList */
void removeRegex(FunctionArgs *fa) { // {{{
	fa->matchedOn[fa->matches[1].rm_eo] = '\0';
	if(rlist_remove(regexList, fa->matchedOn + fa->matches[1].rm_so))
		send(fa->target, "%s: Okay!", fa->name);
	else
		send(fa->target, "%s: I don't know anything about that", fa->name);
} // }}}

// Container for information necessary to generate markov chains
Markov *markovGenerator = NULL;

/* markov will eventually print markov chains generated from previous input. */
void markov(FunctionArgs *fa) { // {{{
	if(fa->matchCount == 0) {
		// if we didn't recieve an argument, print usage
		send(fa->target, "%s: Usage: markov <word>", fa->name);
	} else {
		char *msg = markov_fetch(markovGenerator,
				fa->matchedOn + fa->matches[1].rm_so, 4096);
		if(!msg) {
			send(fa->target, "%s: Did not match! (%s)", fa->name,
					fa->matchedOn + fa->matches[1].rm_so);
		} else {
			send(fa->target, "%s: %s", fa->name, msg + ((msg[0] == ' ') ? 1 : 0));
			free(msg);
		}
	}
} // }}}

// FUNCREG should not be used directly
#define FUNCREGX(x, y) { #x, #y, 1, 0, NULL, &x }
/* There are two types of functions:
 * 	A: Has to have some sort of argument after it
 * 	B: May or may not have argumens after it
 * 		the function must make sure that the first token is what it expects
 * 	C: Either the function name is by itself, or it is type A
 */
#define FUNCTIONA(x)    FUNCREGX(x, ^x (.*)$)
#define FUNCTIONB(x)    FUNCREGX(x, ^x.*$)
#define FUNCTIONC(x)    FUNCREGX(x, ^x ## $), FUNCTIONA(x)

FuncStruct functions[] = {
	// Type A functions
	FUNCTIONA(declare),
	FUNCTIONA(increment), { "++", "^ *\\+\\+ *(.*)$", 1, 0, NULL, &increment },
	FUNCTIONA(decrement), { "--",     "^ *-- *(.*)$", 1, 0, NULL, &decrement },
	{ "delete", "^ *delete *(.*)$", 1, 0, NULL, &deleteVariable },

	// Type B functions
	FUNCTIONB(set), FUNCTIONB(help),

	// Type C functions
	FUNCTIONC(wave),
	{ "wave",    "^o/$", 0, 0, NULL, &wave },
	{ "wave", "^o/ .*$", 0, 0, NULL, &wave },
	{ "wave",    "^\\\\o$", 0, 0, NULL, &wave },
	{ "wave", "^\\\\o .*$", 0, 0, NULL, &wave },
	{ "<3", "^<3$", 0, 0, NULL, &lessThanThree },
	FUNCTIONC(fish), FUNCTIONC(fishes), FUNCTIONC(sl), FUNCTIONC(dubstep),
	FUNCTIONC(list), FUNCTIONC(markov),

	// Entirely special type functions
	{ "or", "^(.*) or (.*)$", 2, 0, NULL, &eitherOr },
	{ "is", "^(.*) is (.*)$", 2, 0, NULL, &addRegex },
	{ "forget", "^forget (.*)$", 1, 0, NULL, &removeRegex },

	// End of functions marker
	{ NULL, NULL, 0, 0, NULL, NULL },
};

/* Allocate memory for function regex and compile them */
int setupFunctions() { // {{{
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
} // }}}

/* Free memory associated with functions and their regex */
int deinitFunctions() { // {{{
	for(int i = 0; functions[i].name && functions[i].f; ++i) {
		if(functions[i].r)
			regfree(functions[i].r);
		free(functions[i].r);
	}
	return 0;
} // }}}

/* main runs through a loop getting input and sending output. We logFile
 * everything to a file name *lfname. See internals for commands recognized
 */
int main(int argc, char **argv) {
	int markovMode = 0, seed = 0;
	if(argc > 1) {
		if(!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
			printf("Usage: %s", argv[0]);
			printf("Usually, this is run by conbot. If you want to run it");
			printf(" manually, you must type standard IRC broadcast messages\n");
			printf("If the second argument is an integer, it is used as a random");
			printf(" seed. If it is anything else, markov mode will be entered\n");
			return 0;
		} else {
			seed = atoi(argv[0]);
			if(!seed) {
				markovMode = 1;
				fprintf(stderr, "Entering markov mode\n");
			}
		}
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
	if(seed)
		srand(atoi(argv[1]));

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

	// if we fail to create an RList head, abort
	if((regexList = rlist_create()) == NULL) {
		fprintf(stderr, "Could not create regex list!\n");
		return 1;
	}

	// if we fail to create a Markov, abort
	if((markovGenerator = markov_create(2)) == NULL) {
		fprintf(stderr, "Could not create markov generator!\n");
		return 1;
	}

	// If we fail to open the logFile in append mode, abort
	if(!initLogFile())
		return 1;

	// print separator to logFile
	lprintf("------------------------------------\n");

	// Setup the command start string, abort on failure
	// TODO: regex-ify this thing
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

	// try to read in old regex
	count = rlist_read(regexList, regexDumpFileName);
	if(count > 0) {
		send(owner, "Read in %d regex", count);
	}

	// try to read in old markov chain generator
	count = markov_read(markovGenerator, markovDumpFileName);
	if(count > 0)
		send(owner, "Read in %d markov chain entries", count);
	else if(count)
		send(owner, "Error code reading markov chain: %d", count);

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
					toUs, msgp, 0, mptr, varsMap, confMap
				};

				int matched = 0;
				if(!markovMode) {
					//fprintf(stderr, "Matching on \"%s\"\n", msgp);
					for(int i = 0; !matched && functions[i].f; ++i) {
						fail = regexec(functions[i].r,
								msgp, functions[i].r->re_nsub + 1, mptr, 0);
						if(!fail) {
							matched = 1;
							fargs.matchCount = functions[i].r->re_nsub;
							functions[i].f(&fargs);
						}
					}
				}

				// none of the standard functions matched
				if(!matched) {
					// reload stops this instance, parent conbot starts new one
					if(!strcmp(tok, "reload") && !strcmp(name, owner) && toUs) {
						lprintf("Got message to restart...\n");
						done = 1;
					// we should write out the var map to a file
					} else if(!strcmp(tok, "wvar") && !strcmp(name, owner) && toUs) {
						tok = strtok(NULL, " ");
						if(!tok) {
							send(fargs.target, "%s: Must specify a file name!\n",
									fargs.name);
						} else {
							int res = bmap_writeDot(varsMap, tok);
							send(fargs.target, "%s: Wrote var map to %s (%d)\n",
									fargs.name, tok, res);
						}
					// token after cstart does not match any command
					} else {
						char *is = rlist_match(regexList, msg);
						if(is) {
							send(fargs.target, "%s", is);
						} else {
							markov_insert(markovGenerator, msg);
							//fprintf(stderr, "inserted \"%s\" into mgen", msg);
							//fprintf(stderr, "Could not match!\n");
							// msg ends with question mark, guess an answer
							if(toUs && (strlen(msg) > 0) &&
								(msg[strlen(msg) - 1] == '?')) {
								send(fargs.target, "%s: %s", name,
										((rand() % 2) ? "Yes" : "No"));
							}
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
	bmap_free(varsMap);

	// free conf map
	// TODO: use this
	bmap_free(confMap);

	// try to dump out regex for reload
	count = rlist_dump(regexList, regexDumpFileName);
	if(count > 0) {
		send(owner, "Dumped %d regex", count);
	}
	rlist_free(regexList);

	// try to dump the markov chain generator to a file/dir
	count = markov_dump(markovGenerator, markovDumpFileName);
	if(count > 0) {
		send(owner, "Dumped %d markov chain entries", count);
	}
	markov_free(markovGenerator);

	// free regex objects used in main
	regfree(&pmsgRegex);
	regfree(&joinRegex);

	// free function regex objects
	deinitFunctions();

	// free cstart string
	free(cstart);

	fflush(stdout);
	lflush();

	// de initialize log file
	deinitLogFile();

	// close stdin/stdout just to make sure conbot knows we stopped
	close(0);
	close(1);
	return 0;
}

