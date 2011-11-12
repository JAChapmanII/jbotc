#include "bmap.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PBSIZE 256

void bmapn_fixHeight(BMap_Node *bmn);

BMap_Node *bmapn_add(BMap_Node *bmn, char *k, char *v);
BMap_Node *bmapn_find(BMap_Node *bmn, char *k);
BMap_Node *bmapn_min(BMap_Node *bmn);

BMap_Node *bmapn_erase(BMap_Node *bmn, char *k);

BMap_Node *bmapn_balance(BMap_Node *bmn);
BMap_Node *bmapn_rightRotation(BMap_Node *n);
BMap_Node *bmapn_leftRotation(BMap_Node *n);

int bmapn_depth(BMap_Node *bmn);
int bmapn_size(BMap_Node *bmn);

BMap *bmap_create(void) { // {{{
	BMap *bmap = malloc(sizeof(BMap));
	if(!bmap)
		return NULL;
	bmap->root = NULL;
	return bmap;
} // }}}
void bmap_free(BMap *bmap) { // {{{
	if(!bmap)
		return;
	if(bmap->root)
		bmapn_free(bmap->root);
	free(bmap);
} // }}}

BMap_Node *bmapn_create(char *key, char *val) { // {{{
	if(!key || !val)
		return NULL;
	BMap_Node *bmn = malloc(sizeof(BMap_Node));
	if(!bmn)
		return NULL;
	bmn->left = bmn->right = NULL;
	bmn->height = 1;
	bmn->key = malloc(strlen(key) + 1);
	bmn->val = malloc(strlen(val) + 1);
	if(!bmn->key || !bmn->val) {
		bmapn_free(bmn);
		return NULL;
	}
	strcpy(bmn->key, key);
	strcpy(bmn->val, val);
	return bmn;
} // }}}
void bmapn_free(BMap_Node *bmn) { // {{{
	if(!bmn)
		return;
	if(bmn->left)
		bmapn_free(bmn->left);
	if(bmn->right)
		bmapn_free(bmn->right);
	free(bmn->key);
	free(bmn->val);
	free(bmn);
} // }}}

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
BMap_Node *bmapn_add(BMap_Node *bmn, char *k, char *v) { // {{{
	if(!bmn || !k || !v) return bmn; // TODO: this was just return; ....
	int cmp = strcmp(bmn->key, k);
	// TODO: error on this condition? This makes _set obsolete
	if(!cmp) {
		if(strcmp(bmn->val, v)) {
			free(bmn->val);
			bmn->val = malloc(strlen(k) + 1);
			strcpy(bmn->val, v);
		}
		return bmn;
	}

	BMap_Node **child;
	if(cmp > 0)
		child = &bmn->left;
	else
		child = &bmn->right;

	if(!*child)
		*child = bmapn_create(k, v);
	else
		*child = bmapn_add(*child, k, v);

	bmapn_fixHeight(bmn);
	return bmapn_balance(bmn);
} // }}}

uint8_t bmapn_height(BMap_Node *bmn) { // {{{
	if(!bmn)
		return 0;
	return bmn->height;
} // }}}

// TODO: implement
int bmap_set(BMap *bmap, char *k, char *v);

BMap_Node *bmap_min(BMap *bmap) { // {{{
	if(!bmap || !bmap->root)
		return NULL;
	return bmapn_min(bmap->root);
} // }}}
BMap_Node *bmapn_min(BMap_Node *bmn) { // {{{
	if(!bmn)
		return NULL;
	if(bmn->left)
		return bmapn_min(bmn->left);
	return bmn;
} // }}}

int bmap_erase(BMap *bmap, char *k) { // {{{
	if(!bmap || !k)
		return 1;
	bmap->root = bmapn_erase(bmap->root, k);
	return 0;
} // }}}
BMap_Node *bmapn_erase(BMap_Node *bmn, char *k) { // {{{
	if(!bmn || !k)
		return NULL;
	int cmp = strcmp(bmn->key, k);
	if(!cmp) {
		if(!bmn->left && !bmn->right) {
			bmapn_free(bmn);
			return NULL;
		}
		if(bmn->left && !bmn->right) {
			BMap_Node *l = bmn->left;
			bmn->left = NULL;
			bmapn_free(bmn);
			return l;
		}
		if(!bmn->left && bmn->right) {
			BMap_Node *r = bmn->right;
			bmn->right = NULL;
			bmapn_free(bmn);
			return r;
		}
		// replace with min from right tree
		BMap_Node *min = bmapn_min(bmn->right);
		if(!min) {
			; // TODO: panic
		}
		BMap_Node *m = bmapn_create(min->key, min->val);
		if(!m) {
			; // TODO: panic
		}
		// this should be one of the simple cases above
		bmn->right = bmapn_erase(bmn->right, min->key);

		m->left = bmn->left;
		m->right = bmn->right;
		m->height = bmn->height;

		bmn->left = bmn->right = NULL;
		bmapn_free(bmn);

		bmapn_fixHeight(m);
		return bmapn_balance(m);
	}
	if(cmp > 0) {
		if(!bmn->left)
			return bmn;
		bmn->left = bmapn_erase(bmn->left, k);
	} else {
		if(!bmn->right)
			return bmn;
		bmn->right = bmapn_erase(bmn->right, k);
	}
	bmapn_fixHeight(bmn);
	return bmapn_balance(bmn);
} // }}}

void bmapn_fixHeight(BMap_Node *bmn) { // {{{
	if(!bmn)
		return;
	int lh = bmapn_height(bmn->left),
		rh = bmapn_height(bmn->right);
	bmn->height = ((lh > rh) ? lh : rh) + 1;
} // }}}

BMap_Node *bmapn_balance(BMap_Node *bmn) { // {{{
	int lh = bmapn_height(bmn->left), rh = bmapn_height(bmn->right);
	if(rh - lh > 1) {
		lh = bmapn_height(bmn->right->left);
		rh = bmapn_height(bmn->right->right);
		if(lh > rh)
			bmn->right = bmapn_rightRotation(bmn->right);
		bmn = bmapn_leftRotation(bmn);
	} else if(lh - rh > 1) {
		lh = bmapn_height(bmn->left->left);
		rh = bmapn_height(bmn->left->right);
		if(rh > lh)
			bmn->left = bmapn_leftRotation(bmn->left);
		bmn = bmapn_rightRotation(bmn);
	}
	return bmn;
} // }}}

BMap_Node *bmapn_rightRotation(BMap_Node *n) { // {{{
	BMap_Node *nr = n->left, *c = n->left->right;
	nr->right = n;
	n->left = c;

	bmapn_fixHeight(n);
	bmapn_fixHeight(nr);
	return nr;
} // }}}
BMap_Node *bmapn_leftRotation(BMap_Node *n) { // {{{
	BMap_Node *nr = n->right, *b = n->right->left;
	nr->left = n;
	n->right = b;

	bmapn_fixHeight(n);
	bmapn_fixHeight(nr);
	return nr;
} // }}}

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
	if(cmp > 0)
		return bmapn_find(bmn->left, k);
	return bmapn_find(bmn->right, k);
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

// TODO: the way this works, there is no way to remove variables but to stop
// conbot and then remove them from the .dat file. This should be corrected,
// probably.
// TODO: Allow deletion from a bmap
int bmap_read(BMap *bmap, char *fileName) {
	if(!bmap || !fileName)
		return 0;

	FILE *dumpFile = fopen(fileName, "r");
	if(!dumpFile)
		return 0;

	int count = 0;
	while(!feof(dumpFile)) {
		char key[PBSIZE], val[PBSIZE];
		if(!fscanf(dumpFile, "%s ", key))
			break;
		if(!fscanf(dumpFile, "%s ", val))
			break;
		bmap_add(bmap, key, val);
		count++;
	}
	fclose(dumpFile);
	return count;
}

// TODO: somehow we dump the same variable multiple times...
// TODO: wasn't this fixed somewhere? >_>
int bmapn_dump(BMap_Node *bmn, FILE *dumpFile) {
	if(!bmn)
		return 0;

	// TODO: not space when keys/values can have spaces?
	fprintf(dumpFile, "%s %s\n", bmn->key, bmn->val);
	int count = 1;
	count += bmapn_dump(bmn->left, dumpFile);
	count += bmapn_dump(bmn->right, dumpFile);
	return count;
}

int bmap_dump(BMap *bmap, char *fileName) {
	if(!bmap || !fileName)
		return 0;

	FILE *dumpFile = fopen(fileName, "w");
	if(!dumpFile)
		return 0;

	int count = bmapn_dump(bmap->root, dumpFile);
	fclose(dumpFile);
	return count;
}

int bmapn_writeDot(BMap_Node *bmn, FILE *of, int nullCount) { // {{{
	if(!of || !bmn)
		return 0;
	fprintf(of, "\t\"%s = %s\";\n", bmn->key, bmn->val);
	if(bmn->left) {
		fprintf(of, "\t\"%s = %s\" -> \"%s = %s\";\n",
				bmn->key, bmn->val, bmn->left->key, bmn->left->val);
		nullCount += bmapn_writeDot(bmn->left, of, nullCount);
	} else {
		fprintf(of, "null%d [shape=point]\n", nullCount);
		fprintf(of, "\t\"%s = %s\" -> null%d\n",
				bmn->key, bmn->val, nullCount);
		nullCount++;
	}
	if(bmn->right) {
		fprintf(of, "\t\"%s = %s\" -> \"%s = %s\";\n",
				bmn->key, bmn->val, bmn->right->key, bmn->right->val);
		nullCount += bmapn_writeDot(bmn->right, of, nullCount);
	} else {
		fprintf(of, "null%d [shape=point]\n", nullCount);
		fprintf(of, "\t\"%s = %s\" -> null%d\n",
				bmn->key, bmn->val, nullCount);
		nullCount++;
	}
	return nullCount;
} // }}}
int bmap_writeDot(BMap *bmap, char *outputName) { // {{{
	if(!bmap || !outputName)
		return 1;

	FILE *of = fopen(outputName, "w");
	if(!of)
		return 2;

	fprintf(of, "digraph BST {\n");
	fprintf(of, "\tnode [fontname=\"Arial\"];\n");
	bmapn_writeDot(bmap->root, of, 0);
	fprintf(of, "}\n");

	fclose(of);
	return 0;
} // }}}

