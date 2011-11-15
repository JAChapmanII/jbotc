#include "dictionary.h"
#include <stdlib.h>

Dictionary *dictionary_create(void) { // {{{
	Dictionary *dict = malloc(sizeof(Dictionary));
	if(!dict)
		return NULL;
	dict->size = 0;
	dict->wi = bmap_create();
	if(!dict->wi) {
		dictionary_free(dict);
		return NULL;
	}
	dict->iw = bmap_create();
	if(!dict->iw) {
		dictionary_free(dict);
		return NULL;
	}
	return dict;
} // }}}
void dictionary_free(Dictionary *dict) { // {{{
	if(!dict)
		return;
	if(dict->wi)
		bmap_free(dict->wi);
	if(dict->iw)
		bmap_free(dict->iw);
	free(dict);
} // }}}

int dictionary_add(Dictionary *dict, char *w) { // {{{
	if(!dict || !w)
		return -1;

	char buf[MAX_KLEN + 1];
	DTYPE v = dict->size + 1;
	if(snprintf(buf, MAX_KLEN + 1, DTYPE_SSTR, v) < 0)
		return -2;

	int fail = bmap_add(dict->wi, w, buf);
	if(fail)
		return fail;
	fail = bmap_add(dict->iw, buf, w);
	if(!fail)
		dict->size++;
	return fail;
} // }}}

DTYPE dictionary_get(Dictionary *dict, char *w) { // {{{
	if(!dict || !w)
		return 0;

	BMap_Node *n = bmap_find(dict->wi, w);
	if(!n)
		return 0;

	DTYPE v;
	if(sscanf(n->val, DTYPE_SSTR, &v) != 1)
		return 0;
	return v;
} // }}}
char *dictionary_lookup(Dictionary *dict, DTYPE w) { // {{{
	if(!dict || !w)
		return NULL;
	if(w > dict->size)
		return NULL;

	char buf[MAX_KLEN + 1];
	if(snprintf(buf, MAX_KLEN + 1, DTYPE_SSTR, w) < 0)
		return NULL;

	BMap_Node *n = bmap_find(dict->iw, buf);
	if(!n)
		return NULL;
	return n->val;
} // }}}

