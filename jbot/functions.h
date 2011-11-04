#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "bmap.h"
#include <stdlib.h>

typedef struct {
	char *name;
	char *hmask;
	char *target;
	char *args;
	int toUs;

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

/* CodeBlock wants a fish... */
void fish(FunctionArgs *fa);

/* CodeBlock wants multiple species of fish */
void fishes(FunctionArgs *fa);

/* WUB WUB WUB WUB WUB */
void dubstep(FunctionArgs *fa);

/* Sidein wants a train! */
void sl(FunctionArgs *fa);

/* Wave if we haven't recently */
void wave(FunctionArgs *fa);

/* Declares variables to remember things. */
void declare(FunctionArgs *fa);

/* Set a variable to remember things. */
void set(FunctionArgs *fa);

/* Increment a variable and create it if it doesn't exist. */
void increment(FunctionArgs *fa);

/* Decrement a variable and create it if it doesn't exist. */
void decrement(FunctionArgs *fa);

#endif // FUNCTIONS_H
