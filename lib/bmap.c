#include "bmap.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PBSIZE 256

/* _stringCopy wraps strcpy and handles simple cases so that valgrind doesn't
 * bitch about invalid read/write sizes in dest/src */
char *_stringCopy(char *dest, char *src) {
	// TODO: what sorts of things does strcpy handle w.r.t errors? We don't
	// know the size of dest or src, so...? TODO
	size_t slen = strlen(src);
	
	size_t dlen = strlen(dest);
	printf("dlen: %lu, slen: %lu\n", dlen, slen);
	if(slen == 0) {
		dest[0] = '\0';
	} else if(slen == 1) {
		dest[0] = src[0];
		dest[1] = '\0';
	} else {
		return strcpy(dest, src);
	}
	return dest;
}
#define strcpy(dest, src) _stringCopy(dest, src)

void bmapn_fixHeight(BMap_Node *bmn);

BMap_Node *bmapn_add(BMap_Node *bmn, char *k, char *v);
BMap_Node *bmapn_find(BMap_Node *bmn, char *k);
BMap_Node *bmapn_min(BMap_Node *bmn);

BMap_Node *bmapn_erase(BMap_Node *bmn, char *k);

BMap_Node *bmapn_balance(BMap_Node *bmn);
BMap_Node *bmapn_rightRotation(BMap_Node *n);
BMap_Node *bmapn_leftRotation(BMap_Node *n);

int bmapn_depth(BMap_Node *bmn);
size_t bmapn_size(BMap_Node *bmn);

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
	bmn->key = bmn->val = NULL;
	// round sizes to multiples of 8, so we don't get strange valgrind errors
	// inside of sscanf. I suppose it isn't a problem (at least most of the
	// time), but at max we're using up an extra 7 bytes per key/val which
	// probably isn't that much... I hope XD TODO: look into that
	bmn->key = malloc(((strlen(key) + 1) + 7) / 8);
	bmn->val = malloc(((strlen(val) + 1) + 7) / 8);
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
			// TODO: is this really a good way to do this?
			size_t vlen = strlen(v);
			// if we need more space, realloc
			if(strlen(bmn->val) < vlen) {
				// TODO: actually realloc not free/malloc?
				free(bmn->val);
				// TODO: again, round to 8 bytes. See _create's notes
				bmn->val = malloc(((vlen + 1) + 7) / 8);
			}
			// copy over new value
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

size_t bmap_size(BMap *bmap) { /* {{{ */
	if(!bmap->root)
		return 0;
	return bmapn_size(bmap->root);
} /* }}} */
size_t bmapn_size(BMap_Node *bmn) { /* {{{ */
	if(!bmn)
		return 0;
	/* NULL childrne are handled above */
	return bmapn_size(bmn->left) + bmapn_size(bmn->right) + 1;
} /* }}} */

size_t writeSizeT(FILE *outFile, size_t toWrite) { // {{{
	if(!outFile)
		return 0;

	uint8_t buf[sizeof(size_t)] = { 0 };
	for(size_t i = 0; i < sizeof(size_t); ++i)
		buf[i] = (toWrite >> (8*i)) & 0xff;

	return fwrite(buf, 1, sizeof(size_t), outFile);
} // }}}
size_t readSizeT(FILE *inFile) { // {{{
	uint8_t buf[sizeof(size_t)] = { 0 };
	size_t read = fread(buf, 1, sizeof(size_t), inFile);
	if(read != sizeof(size_t))
		return 0;

	size_t result = 0;
	for(size_t i = sizeof(size_t); i > 0; --i)
		result = (result << 8) + buf[i - 1];

	return result;
} // }}}

int bmap_load(BMap *bmap, FILE *inFile) { // {{{
	if(!bmap || !inFile)
		return -1;

	size_t size = readSizeT(inFile);
	if(size == 0)
		return -2;

	size_t rnodes;
	for(rnodes = 0; rnodes < size; ++rnodes) {
		char key[PBSIZE] = { 0 }, val[PBSIZE] = { 0 };
		uint8_t klen = 0, vlen = 0;
		size_t read = 0;

		// attemp to read lengths and key/val
		read = fread(&klen, 1, 1, inFile);
		if(read != 1)
			break;
		read = fread(key, 1, klen, inFile);
		if(read != klen)
			break;
		read = fread(&vlen, 1, 1, inFile);
		if(read != 1)
			break;
		read = fread(val, 1, vlen, inFile);
		if(read != vlen)
			break;

		// null terminate strings
		key[klen] = '\0';
		val[vlen] = '\0';

		// add the node to the bmap
		bmap_add(bmap, key, val);
	}

	if(rnodes != size) {
		fprintf(stderr, "rn: %lu, s: %lu\n", rnodes, size);
		return -3;
	}
	return rnodes;
} // }}}
int bmap_read(BMap *bmap, char *fileName) { // {{{
	if(!bmap || !fileName)
		return 0;

	FILE *dumpFile = fopen(fileName, "r");
	if(!dumpFile)
		return 0;

	int res = bmap_load(bmap, dumpFile);
	fclose(dumpFile);

	return res;
} // }}}

// TODO: somehow we dump the same variable multiple times...
// TODO: wasn't this fixed somewhere? >_>
int bmapn_dump(BMap_Node *bmn, FILE *dumpFile) { // {{{
	if(!bmn || !dumpFile)
		return 0;

	int count = 1;
	size_t keylength = strlen(bmn->key), vallength = strlen(bmn->val);
	if((keylength >= (2 << 8)) || (vallength >= (2 << 8)))
		count = 0;

	if(count) {
		size_t written;
		uint8_t klen = keylength, vlen = vallength;
		written = fwrite(&klen, 1, 1, dumpFile);
		if(written != 1) {
			; // TODO: any way to recover? seek backward?
		}
		written = fwrite(bmn->key, 1, klen, dumpFile);
		if(written != klen) {
			; // TODO: see above
		}
		written = fwrite(&vlen, 1, 1, dumpFile);
		if(written != 1) {
			; // TODO: see above
		}
		written = fwrite(bmn->val, 1, vlen, dumpFile);
		if(written != vlen) {
			; // TODO: see above
		}
	}

	count += bmapn_dump(bmn->left, dumpFile);
	count += bmapn_dump(bmn->right, dumpFile);
	return count;
} // }}}
int bmap_write(BMap *bmap, FILE *outFile) { // {{{
	if(!bmap || !outFile)
		return -1;

	size_t size = bmap_size(bmap);
	if(size < 1)
		return 0;

	if(writeSizeT(outFile, size) != sizeof(size_t))
		return -128;

	int count = bmapn_dump(bmap->root, outFile);
	return count;
} // }}}
int bmap_dump(BMap *bmap, char *fileName) { // {{{
	if(!bmap || !fileName)
		return 0;

	FILE *dumpFile = fopen(fileName, "wb");
	if(!dumpFile)
		return 0;

	int count = bmap_write(bmap, dumpFile);
	fclose(dumpFile);
	return count;
} // }}}

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

