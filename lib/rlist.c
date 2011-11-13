#include "rlist.h"
#include "defines.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_SUB 32
#define MAX_LEN 128

RList *rlist_create(void) { // {{{
	RList *rlst = malloc(sizeof(RList));
	if(!rlst)
		return NULL;
	rlst->regex = NULL;
	rlst->data = NULL;
	rlst->next = NULL;
	rlst->r = NULL;
	return rlst;
} // }}}
void rlist_free(RList *rlst) { // {{{
	if(!rlst)
		return;
	if(rlst->next)
		rlist_free(rlst->next);
	free(rlst->data);
	free(rlst->regex);
	regfree(rlst->r);
	free(rlst->r);
	free(rlst);
} // }}}

int rlist_size(RList *rlst) { // {{{
	if(!rlst || !rlst->next)
		return 0;
	int size = 0;
	while(rlst->next)
		++size, rlst = rlst->next;
	return size;
} // }}}

char *regError(int errcode, regex_t *compiled) { // {{{
	size_t l = regerror(errcode, compiled, NULL, 0);
	char *buffer = malloc(l + 1);
	if(!buffer)
		return "Couldn't malloc space for regex errror!";
	regerror(errcode, compiled, buffer, l);
	return buffer;
} // }}}

char *rlist_add(RList *rlst, char *regex, char *data) { // {{{
	if(!rlst || !regex || !data)
		return "Null parameter error.";
	RList *next = rlist_create();
	int datalen = strlen(data),
		regexlen = strlen(regex);
	if((datalen > MAX_LEN) || (regexlen > MAX_LEN)) {
		rlist_free(next);
		return "Data or regex exceeds max length.";
	}
	next->regex = malloc(regexlen + 1);
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
	return NULL;
} // }}}

char *rlist_match(RList *rlst, char *msg) { // {{{
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
} // }}}

int rlist_dump(RList *rlst, char *fileName) {
	if(!rlst || !fileName || !rlst->next)
		return 0;

	FILE *dumpFile = fopen(fileName, "w");
	if(!dumpFile)
		return 0;

	int count = 0;
	while(rlst->next) {
		rlst = rlst->next;
		fprintf(dumpFile, "%c%s%c%s",
				(char)strlen(rlst->regex), rlst->regex,
				(char)strlen(rlst->data), rlst->data);
		count++;
	}
	fclose(dumpFile);
	return count;
}

int rlist_read(RList *rlst, char *fileName) {
	if(!rlst || !fileName)
		return 0;

	FILE *dumpFile = fopen(fileName, "rb");
	if(!dumpFile)
		return 0;

	// fast forward to end of rlst
	while(rlst->next)
		rlst = rlst->next;

	int count = 0;
	char regex[BSIZE], data[BSIZE];
	uint8_t rlen, dlen;
	while(!feof(dumpFile)) {
		// TODO: handle these read errors?
		size_t read = fread(&rlen, 1, 1, dumpFile);
		if(read != 1)
			break;
		read = fread(regex, 1, rlen, dumpFile);
		if(read != rlen)
			break;
		regex[read] = '\0';
		read = fread(&dlen, 1, 1, dumpFile);
		if(read != 1)
			break;
		read = fread(data, 1, dlen, dumpFile);
		if(read != dlen)
			break;
		data[read] = '\0';

		char *msg = rlist_add(rlst, regex, data);
		if(msg) {
			fprintf(stderr, "Could not add regex! \"%s\"\n", regex);
			fprintf(stderr, "Error message is: %s\n", msg);
		} else {
			count++;
			rlst = rlst->next;
		}
	}
	fclose(dumpFile);
	return count;
}


