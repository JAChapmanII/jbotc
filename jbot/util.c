#include "util.h"
#include "defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

FILE *logFile = NULL;

// the name of the file to dump/read variables from
const char *dumpfname = "jbotc.dat";

char *getRegError(int errcode, regex_t *compiled) {
	size_t l = regerror(errcode, compiled, NULL, 0);
	char *buffer = malloc(l + 1);
	if(!buffer)
		return "Couldn't malloc space for regex errror!";
	regerror(errcode, compiled, buffer, l);
	return buffer;
}

// TODO: the way this works, there is no way to remove variables but to stop
// conbot and then remove them from the .dat file. This should be corrected,
// probably.
// TODO: Allow deletion from a bmap
int readDump(BMap *readTo) {
	FILE *dumpFile = fopen(dumpfname, "r");
	if(!dumpFile)
		return 0;
	int count = 0;
	while(!feof(dumpFile)) {
		char key[PBSIZE], val[PBSIZE];
		if(!fscanf(dumpFile, "%s ", key))
			break;
		if(!fscanf(dumpFile, "%s ", val))
			break;
		bmap_add(readTo, key, val);
		count++;
	}
	fclose(dumpFile);
	return count;
}

// TODO: somehow we dump the same variable multiple times...
int dumpVars_(BMap_Node *bmn, FILE *dumpFile) {
	if(!bmn)
		return 0;
	// TODO: not space when keys/values can have spaces?
	fprintf(dumpFile, "%s %s\n", bmn->key, bmn->val);
	int count = 1;
	count += dumpVars_(bmn->left, dumpFile);
	count += dumpVars_(bmn->right, dumpFile);
	return count;
}

int dumpVars(BMap *dumpFrom) {
	FILE *dumpFile = fopen(dumpfname, "w");
	if(!dumpFrom)
		return 0;
	int count = dumpVars_(dumpFrom->root, dumpFile);
	fclose(dumpFile);
	return count;
}

int initLogFile() {
	logFile = fopen(lfname, "a");
	if(!logFile) {
		fprintf(stderr, "Could not open logFile file!\n");
		return 0;
	}
	return 1;
}

void send(const char *target, char *format, ...) {
	va_list args;
	va_start(args, format);

	char buf[BSIZE];
	vsnprintf(buf, BSIZE, format, args);

	printf("PRIVMSG %s :%s\n", target, buf);
	if(!logFile)
		initLogFile();
	fprintf(logFile, " -> @%s :%s\n", target, buf);

	va_end(args);
}

void lprintf(char *format, ...) {
	if(!logFile)
		return;
	va_list args;
	va_start(args, format);
	vfprintf(logFile, format, args);
	va_end(args);
}

void lflush() {
	fflush(logFile);
}

