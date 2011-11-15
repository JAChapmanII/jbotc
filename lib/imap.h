#ifndef IMAP_H
#define IMAP_H

#include "bmap.h"
#include <stdint.h>

#define MAX_KLEN 10
#define DTYPE uint32_t
#define DTYPE_SSTR "%u"

typedef struct {
	BMap *map;
	uint32_t size;
} IMap;

/* Create an empty IMap object */
IMap *imap_create(void);
/* Free space associated with a BMap_Node */
void imap_free(IMap *imap);

/* Add a new key/value pair to the tree */
int imap_add(IMap *bmap, char *k, DTYPE v);
/* Remove a node from the tree */
int imap_erase(IMap *bmap, char *k);

/* Retrieve the value of a key
 * 	note: returns 0 if not found
 */
DTYPE imap_get(IMap *imap, char *k);

#endif // IMAP_H
