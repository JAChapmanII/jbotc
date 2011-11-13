#ifndef MARKOV_H
#define MARKOV_H

#include "bmap.h"

typedef struct {
	int order;
	BMap *ploc;
} Markov;

Markov *markov_create(int order);
void markov_free(Markov *mkv);

void  markov_insert(Markov *mkv, char *str);
char *markov_search(Markov *mkv, char *str);

#endif // MARKOV_H
