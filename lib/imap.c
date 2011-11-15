#include "imap.h"
#include <stdlib.h>

IMap *imap_create(void) { // {{{
	IMap *imap = malloc(sizeof(IMap));
	if(!imap)
		return NULL;
	imap->size = 0;
	imap->map = bmap_create();
	if(!imap->map) {
		imap_free(imap);
		return NULL;
	}
	return imap;
} // }}}
void imap_free(IMap *imap) { // {{{
	if(!imap)
		return;
	if(imap->map)
		bmap_free(imap->map);
	free(imap);
} // }}}

int imap_add(IMap *imap, char *k, DTYPE v) { // {{{
	if(!imap || !k)
		return -1;

	char buf[MAX_KLEN + 1];
	if(snprintf(buf, MAX_KLEN + 1, DTYPE_SSTR, v) < 0)
		return -2;

	int fail = bmap_add(imap->map, k, buf);
	if(!fail)
		imap->size++;
	return fail;
} // }}}
int imap_erase(IMap *imap, char *k) { // {{{
	if(!imap || !k)
		return -1;
	if(!bmap_find(imap->map, k))
		return -2;
	int fail = bmap_erase(imap->map, k);
	if(!fail)
		imap->size--;
	return fail;
} // }}}

DTYPE imap_get(IMap *imap, char *k) { // {{{
	if(!imap || !k)
		return 0;

	BMap_Node *n = bmap_find(imap->map, k);
	if(!n)
		return 0;

	DTYPE v;
	if(sscanf(n->val, DTYPE_SSTR, &v) != 1)
		return 0;
	return v;
} // }}}

