#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "bmap.h"
#include "util.h"
#include <stdlib.h>

typedef struct {
	char *name;
	char *hmask;
	char *target;
	char *args;

	BMap *vars;
	BMap *conf;
} FunctionArgs;

typedef void (*Function)(FunctionArgs *fa);
typedef struct {
	char *name;
	Function f;
} FuncStruct;

/* markov will eventually print markov chains generated from previous input. */
void markov(FunctionArgs *fa);

/*
// CodeBlock wants a fish...
} else if(!strcmp(tok, "fish")) {
	send(chan, "%s: %s", name, ((rand() % 2) ? "><>" : "<><"));

// CodeBlock wants multiple species of fish
} else if(!strcmp(tok, "fishes")) {
	send(chan, "%s: ><> <>< <><   ><> ><>", name);

// WUB WUB WUB WUB WUB
} else if(!strcmp(tok, "dubstep")) {
	send(chan, "%s: WUB WUB WUB", name);
*/

/* Declares variables to remember things. */
void declareVariable(FunctionArgs *fa);

/* Set a variable to remember things. */
void setVariable(FunctionArgs *fa);

/* Increment a variable and create it if it doesn't exist. */
void incrementVariable(FunctionArgs *fa);

/* Decrement a variable and create it if it doesn't exist. */
void decrementVariable(FunctionArgs *fa);

#endif // FUNCTIONS_H
