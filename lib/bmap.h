#ifndef BMAP_H
#define BMAP_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* BMap_Node, a node in an AVL tree */
typedef struct BMap_Node {
	char *key;
	char *val;
	struct BMap_Node *left;
	struct BMap_Node *right;
	uint8_t height;
} BMap_Node;

/* A string -> string map backed by an AVL tree */
typedef struct {
	BMap_Node *root;
} BMap;

/* Create an empty BMap */
BMap *bmap_create(void);
/* Free all memory associated with a BMap */
void bmap_free(BMap *bmap);

/* Create a BMap_Node object */
BMap_Node *bmapn_create(char *key, char *val);
/* Free space associated with a BMap_Node */
void bmapn_free(BMap_Node *bmn);

/* Add a new key/value pair to the tree
 * 	returns 1 on failure, 0 otherwise
 */
int bmap_add(BMap *bmap, char *k, char *v);
/* Update the value in an already existing node */
int bmap_set(BMap *bmap, char *k, char *v);
/* Remove a node from the tree */
int bmap_erase(BMap *bmap, char *k);

/* Search BMap for a specific node */
BMap_Node *bmap_find(BMap *bmap, char *k);

/* Search a BMap for the minimum element */
BMap_Node *bmap_min(BMap *bmap);

/* Recursively compute the total nodes in a BMap */
size_t bmap_size(BMap *bmap);

/* Write a bmap to a file */
int bmap_write(BMap *bmap, FILE *outFile);
/* Try to open a file and write the BMap to it */
int bmap_dump(BMap *bmap, char *fileName);

/* Read a BMap from a file */
int bmap_load(BMap *bmap, FILE *inFile);
/* Try to open a file and load the BMap from it */
int bmap_read(BMap *bmap, char *fileName);

/* Write this BMap out as a graphviz .dot file */
int bmap_writeDot(BMap *bmap, char *outputName);

#endif /* BMAP_H */
