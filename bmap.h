#ifndef BMAP_H
#define BMAP_H

typedef struct BMap_Node {
	char *key;
	char *val;
	struct BMap_Node *left;
	struct BMap_Node *right;
} BMap_Node;

typedef struct {
	char *k, *v;
} BMap_Entry;

typedef struct {
	BMap_Node *root;
} BMap;

BMap *bmap_create(void);
void bmap_free(BMap *bmap);

BMap_Node *bmapn_create(char *key, char *val);
void bmapn_free(BMap_Node *bmn);

int bmap_add(BMap *bmap, char *k, char *v);
int bmap_set(BMap *bmap, char *k, char *v);

BMap_Node *bmap_find(BMap *bmap, char *k);

int bmap_depth(BMap *bmap);
int bmap_size(BMap *bmap);

#endif /* BMAP_H */
