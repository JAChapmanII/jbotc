#include <stdlib.h>
#include <string.h>

#include "bmap.h"

BMap_Node *bmapn_add(BMap_Node *bmn, char *k, char *v);
BMap_Node *bmapn_rightRotation(BMap_Node *n);
BMap_Node *bmapn_leftRotation(BMap_Node *n);
BMap_Node *bmapn_find(BMap_Node *bmn, char *k);

int bmapn_depth(BMap_Node *bmn);
int bmapn_size(BMap_Node *bmn);

BMap *bmap_create(void) { /* {{{ */
	BMap *bmap;
	bmap = malloc(sizeof(BMap));
	if(!bmap)
		return NULL;
	bmap->root = NULL;
	return bmap;
} /* }}} */
void bmap_free(BMap *bmap) { /* {{{ */
	if(!bmap)
		return;
	if(bmap->root)
		bmapn_free(bmap->root);
	free(bmap);
} /* }}} */

BMap_Node *bmapn_create(char *key, char *val) { /* {{{ */
	BMap_Node *bmn;
	if(!key || !val)
		return NULL;
	bmn = malloc(sizeof(BMap_Node));
	if(!bmn)
		return NULL;
	bmn->left = bmn->right = NULL;
	bmn->key = malloc(strlen(key) + 1);
	bmn->val = malloc(strlen(val) + 1);
	if(!bmn->key || !bmn->val) {
		bmapn_free(bmn);
		return NULL;
	}
	strcpy(bmn->key, key);
	strcpy(bmn->val, val);
	return bmn;
} /* }}} */
void bmapn_free(BMap_Node *bmn) { /* {{{ */
	if(!bmn)
		return;
	if(bmn->left)
		bmapn_free(bmn->left);
	if(bmn->right)
		bmapn_free(bmn->right);
	free(bmn->key);
	free(bmn->val);
	free(bmn);
} /* }}} */

/* TODO: error handling */
int bmap_add(BMap *bmap, char *k, char *v) {
	if(!k || !v)
		return 1;
	if(!bmap->root) {
		bmap->root = bmapn_create(k, v);
		if(!bmap->root)
			return 1;
		return 0;
	}
	bmap->root = bmapn_add(bmap->root, k, v);
	return 0;
}
BMap_Node *bmapn_add(BMap_Node *bmn, char *k, char *v) { /* {{{ */
	int cmp, ld, rd;
	BMap_Node **child;
	if(!bmn || !k || !v) return bmn; /* TODO: this was just return; .... */
	cmp = strcmp(bmn->key, k);
	if(!cmp) {
		if(strcmp(bmn->val, v)) {
			free(bmn->val);
			bmn->val = malloc(strlen(k) + 1);
			strcpy(bmn->val, v);
		}
	}

	if(cmp < 0)
		child = &bmn->left;
	else
		child = &bmn->right;

	if(!*child)
		*child = bmapn_create(k, v);
	else
		*child = bmapn_add(*child, k, v);

	ld = bmapn_depth(bmn->left);
	rd = bmapn_depth(bmn->right);
	if(rd - ld > 1) {
		int r_ld = bmapn_depth(bmn->right->left), r_rd = bmapn_depth(bmn->right->right);
		if(r_ld > r_rd)
			bmn->right = bmapn_rightRotation(bmn->right);
		bmn = bmapn_leftRotation(bmn);
	} else if(ld - rd > 1) {
		int l_ld = bmapn_depth(bmn->left->left), l_rd = bmapn_depth(bmn->left->right);
		if(l_rd > l_ld)
			bmn->left = bmapn_leftRotation(bmn->left);
		bmn = bmapn_rightRotation(bmn);
	}

	return bmn;
} /* }}} */

/* TODO: implement */
int bmap_set(BMap *bmap, char *k, char *v);

BMap_Node *bmapn_rightRotation(BMap_Node *n) { /* {{{ */
	BMap_Node *nr = n->left, *c = n->left->right;

	nr->right = n;
	n->left = c;

	return nr;
} /* }}} */
BMap_Node *bmapn_leftRotation(BMap_Node *n) { /* {{{ */
	BMap_Node *nr = n->right, *b = n->right->left;

	nr->left = n;
	n->right = b;

	return nr;
} /* }}} */

BMap_Node *bmap_find(BMap *bmap, char *k) { /* {{{ */
	if(!bmap->root || !k)
		return NULL;
	return bmapn_find(bmap->root, k);
} /* }}} */
BMap_Node *bmapn_find(BMap_Node *bmn, char *k) { /* {{{ */
	int cmp;
	if(!bmn || !k) return NULL;
	cmp = strcmp(bmn->key, k);
	if(!cmp)
		return bmn;
	if(cmp < 0)
		return bmapn_find(bmn->left, k);
	return bmapn_find(bmn->right, k);
} /* }}} */

int bmap_depth(BMap *bmap) { /* {{{ */
	if(!bmap->root)
		return 0;
	return bmapn_depth(bmap->root);
} /* }}} */
int bmapn_depth(BMap_Node *bmn) { /* {{{ */
	int ld, rd;
	if(!bmn)
		return 0;

	/* if children are NULL, above check handles it */
	ld = bmapn_depth(bmn->left);
	rd = bmapn_depth(bmn->right);

	return ((ld > rd) ? ld : rd) + 1;
} /* }}} */

int bmap_size(BMap *bmap) { /* {{{ */
	if(!bmap->root)
		return 0;
	return bmapn_size(bmap->root);
} /* }}} */
int bmapn_size(BMap_Node *bmn) { /* {{{ */
	if(!bmn)
		return 0;
	/* NULL childrne are handled above */
	return bmapn_size(bmn->left) + bmapn_size(bmn->right) + 1;
} /* }}} */


/* TODO: scrap?
bMap *addNodes(bMap *bm, entry nodes[]) {
	while(nodes->k)
		bm = addNode(bm, nodes->k, nodes->v), ++nodes;
	return bm;
}
*/

