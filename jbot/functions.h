#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "bmap.h"
#include <stdlib.h>
#include <regex.h>

typedef struct {
	char *name;
	char *hmask;
	char *target;
	int toUs;

	char *matchedOn;
	int matchCount;
	regmatch_t *matches;

	BMap *vars;
	BMap *conf;
} FunctionArgs;

typedef void (*Function)(FunctionArgs *fa);
typedef struct {
	char *name;
	char *regx;
	int matchCount;
	int caseSensitive;
	regex_t *r;
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

/* Answers an either/or question */
void eitherOr(FunctionArgs *fa);

/* List variables currently tracked */
void list(FunctionArgs *fa);

#endif // FUNCTIONS_H
