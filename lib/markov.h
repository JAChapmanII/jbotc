#ifndef MARKOV_H
#define MARKOV_H

#include "bmap.h"

typedef struct {
	int order;
	BMap *ploc;
} Markov;

Markov *markov_create(int order);
void markov_free(Markov *mkv);

/* Use this to tokenize and insert a string of arbitrary length */
void markov_insert(Markov *mkv, char *str);
/* Internal helper function, inserts one specific element
 * 	words is a markov->order + 1 char*'s which are the input and output
 */
void markov_push(Markov *mkv, char **words);

/* Use this to fetch some variable length output from a seed
 * 	note: seed must be either
 * 		exactly markov->order words long
 * 		exactly markov->order -1 words long with a starting space */
char *markov_fetch(Markov *mkv, char *seed, int maxLength);
/* Fetch one word using some input */
char *markov_search(Markov *mkv, char *str);

/* Read a markov chain from a file and files in a directory */
int markov_read(Markov *mkv, char *fileName);
/* Dump the markov chain to a file and files in a directory */
int markov_dump(Markov *mkv, char *fileName);

#endif // MARKOV_H
