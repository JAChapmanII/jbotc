#include "util.h"
#include "defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

FILE *logFile = NULL;

char *getRegError(int errcode, regex_t *compiled) {
	size_t l = regerror(errcode, compiled, NULL, 0);
	char *buffer = malloc(l + 1);
	if(!buffer)
		return "Couldn't malloc space for regex errror!";
	regerror(errcode, compiled, buffer, l);
	return buffer;
}

int initLogFile() {
	logFile = fopen(logFileName, "a");
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

