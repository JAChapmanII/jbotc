#include "rlist.h"
#include <stdlib.h>
#include <string.h>

#define MAX_SUB 32

RList *rlist_create(void) {
	RList *rlst = malloc(sizeof(RList));
	if(!rlst)
		return NULL;
	rlst->regex = NULL;
	rlst->data = NULL;
	rlst->next = NULL;
	rlst->r = NULL;
	return rlst;
}
void rlist_free(RList *rlst) {
	if(!rlst)
		return;
	if(rlst->next)
		rlist_free(rlst->next);
	free(rlst->data);
	free(rlst->regex);
	regfree(rlst->r);
	free(rlst->r);
	free(rlst);
}

char *regError(int errcode, regex_t *compiled) {
	size_t l = regerror(errcode, compiled, NULL, 0);
	char *buffer = malloc(l + 1);
	if(!buffer)
		return "Couldn't malloc space for regex errror!";
	regerror(errcode, compiled, buffer, l);
	return buffer;
}

char *rlist_add(RList *rlst, char *regex, char *data) {
	if(!rlst || !regex || !data)
		return "Null parameter error.";
	RList *next = rlist_create();
	next->regex = malloc(strlen(regex) + 1);
	int datalen = strlen(data);
	next->data = malloc(datalen + 1);
	next->r = malloc(sizeof(regex_t));
	if(!next->regex || !next->data || !next->r) {
		rlist_free(next);
		return "Out of memory error.";
	}
	strcpy(next->regex, regex);

	strcpy(next->data, data);
	for(int i = 0; i < datalen; ++i)
		if((data[i] == '\n') || (data[i] == '\t'))
			next->data[i] = ' ';

	int fail = regcomp(next->r, next->regex, REG_EXTENDED);
	if(fail) {
		char *msg = regError(fail, next->r);
		rlist_free(next);
		return msg;
	}

	if(next->r->re_nsub > MAX_SUB) {
		rlist_free(next);
		return "Too many parenthetical subexpressions!";
	}

	while(rlst->next)
		rlst = rlst->next;
	rlst->next = next;
	return "Okay!";
}

char *rlist_match(RList *rlst, char *msg) {
	if(!rlst->next)
		return NULL;

	regmatch_t mptr[MAX_SUB + 1];
	while(rlst->next) {
		rlst = rlst->next;

		int fail = regexec(rlst->r, msg, rlst->r->re_nsub + 1, mptr, 0);
		if(!fail)
			return rlst->data;
	}
	return NULL;
}

