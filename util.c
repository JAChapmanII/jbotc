#include "util.h"
#include "define.h"

FILE *logFile = NULL;

const char *obtainGreeting() {
	return greetings[rand() % GREETING_COUNT];
}

char *getRegError(int errcode, regex_t *compiled) {
	size_t l = regerror(errcode, compiled, NULL, 0);
	char *buffer = malloc(l + 1);
	if(!buffer)
		return "Couldn't malloc space for regex errror!";
	regerror(errcode, compiled, buffer, l);
	return buffer;
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

