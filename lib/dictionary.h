#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "bmap.h"
#include <stdint.h>

#define MAX_KLEN 10
#define DTYPE uint32_t
#define DTYPE_SSTR "%u"

typedef struct {
	BMap *wi;
	BMap *iw;
	uint32_t size;
} Dictionary;

/* Create an empty Dictionary object */
Dictionary *dictionary_create(void);
/* Free space associated with a BMap_Node */
void dictionary_free(Dictionary *dictionary);

/* Add a new key/value pair to the tree */
int dictionary_add(Dictionary *dict, char *w);

/* Retrieve the value of a word
 * 	note: returns 0 if not found
 */
DTYPE dictionary_get(Dictionary *dict, char *w);
/* Lookup the word corresponding to a value */
char *dictionary_lookup(Dictionary *dict, DTYPE w);

#endif // DICTIONARY_H
