#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

#define BSIZE 4096
#define PBSIZE 256

char *getRegError(int errcode, regex_t *compiled) {
	size_t l = regerror(errcode, compiled, NULL, 0);
	char *buffer = malloc(l + 1);
	regerror(errcode, compiled, buffer, l);
	return buffer;
}

void pmsg(char *target, char *msg) {
	printf("PRIVMSG %s %s\n", target, msg);
}

int main(int argc, char **argv) {
	char *nick = "jbotc", *chan = "#zebra", *owner = "jac", *lfname = "jbot.log";
	char str[BSIZE], *tok, *cstart;
	char name[PBSIZE], hmask[PBSIZE], cname[PBSIZE], *msg;
	FILE *log = NULL;

	regex_t pmsgRegex;
	regmatch_t mptr[16];
	int res, done = 0;

	res = regcomp(&pmsgRegex,
			"^:([A-Za-z0-9_]*)!([-@~A-Za-z0-9_\\.]*) PRIVMSG (#[A-Za-z0-9_]*) :(.*)",
			REG_EXTENDED);
	if(res) {
		fprintf(stderr, "Could not compile regex!\n");
		fprintf(stderr, "erromsg: %s\n", getRegError(res, &pmsgRegex));
		return 1;
	}

	log = fopen(lfname, "a");
	if(!log) {
		fprintf(stderr, "Could not open log file!\n");
		return 1;
	}
	fprintf(log, "------------------------------------\n");

	cstart = malloc(strlen(nick) + 2);
	strcpy(cstart, nick);
	strcat(cstart, ":");

	while(!feof(stdin) && !done) {
		if(fgets(str, BSIZE - 1, stdin) == str) {
			str[strlen(str) - 1] = '\0';

			fprintf(log, " <- %s\n", str);
			res = regexec(&pmsgRegex, str, pmsgRegex.re_nsub + 1, mptr, 0);
			if(res == 0) {
				strncpy(name, str + mptr[1].rm_so, mptr[1].rm_eo - mptr[1].rm_so);
				name[mptr[1].rm_eo - mptr[1].rm_so] = '\0';

				strncpy(hmask, str + mptr[2].rm_so, mptr[2].rm_eo - mptr[2].rm_so);
				hmask[mptr[2].rm_eo - mptr[2].rm_so] = '\0';

				strncpy(cname, str + mptr[3].rm_so, mptr[3].rm_eo - mptr[3].rm_so);
				cname[mptr[3].rm_eo - mptr[3].rm_so] = '\0';

				msg = str + mptr[4].rm_so;

				fprintf(log, "PRIVMSG %s :PRIVMSG recieved from %s@%s: %s\n",
						owner, name, cname, msg);
				tok = strtok(msg, " ");
				if(!strcmp(tok, cstart)) {
					tok = strtok(NULL, " ");
					if(!strcmp(tok, "reload")) {
						fprintf(log, "Got message to restart...\n");
						done = 1;
					} else if(!strcmp(tok, "markov")) {
						tok = strtok(NULL, " ");
						if(tok == NULL) {
							printf("PRIVMSG %s :%s: Usage: markov <word>\n", chan, name);
							fprintf(log, " -> PRIVMSG %s :%s: Usage: markov <word>\n",
									chan, name);
						} else {
							printf("PRIVMSG %s :%s: %s\n", chan, name, tok);
							fprintf(log, " -> PRIVMSG %s :%s: %s\n", chan, name, tok);
						}
					} else {
						printf("PRIVMSG %s :%s: ", chan, name);
						fprintf(log, " -> PRIVMSG %s :%s: ", chan, name);
						while(tok != NULL) {
							printf("%s ", tok);
							fprintf(log, "%s ", tok);
							tok = strtok(NULL, " ");
						}
						printf("\n");
						fprintf(log, "\n");
					}
				}
			}
			fflush(stdout);
			fflush(log);
		}
	}

	close(0);
	close(1);
	return 0;
}

