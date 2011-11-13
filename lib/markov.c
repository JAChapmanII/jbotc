#include "markov.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PBSIZE 256
#define BSIZE PBSIZE * 16

Markov *markov_create(int order) { // {{{
	if((order < 1) || (order > 6))
		return NULL;
	Markov *mkv = malloc(sizeof(Markov));
	if(!mkv)
		return NULL;
	mkv->order = order;
	mkv->ploc = bmap_create();
	if(!mkv->ploc) {
		markov_free(mkv);
		return NULL;
	}
	return mkv;
} // }}}

void markov_free_ploc(BMap_Node *bmn) { // {{{
	if(!bmn)
		return;
	markov_free_ploc(bmn->left);
	markov_free_ploc(bmn->right);
	BMap *p;
	sscanf(bmn->val, "%p", (void **)&p);
	bmap_free(p);
} // }}}
void markov_free(Markov *mkv) { // {{{
	if(!mkv)
		return;
	if(mkv->ploc)
		markov_free_ploc(mkv->ploc->root);
	bmap_free(mkv->ploc);
	free(mkv);
} // }}}

void markov_push(Markov *mkv, char **words) { // {{{
	if(!mkv || !words)
		return;

	// concatenate the input together
	char buf[BSIZE] = { 0 };
	for(int i = 0; i < mkv->order; ++i) {
		strcat(buf, words[i]);
		// only add a space after if it's not the last word
		if(i != mkv->order - 1)
			strcat(buf, " ");
	}

	BMap *wmap;
	// try to find the input in ploc
	BMap_Node *m = bmap_find(mkv->ploc, buf);

	if(m == NULL) {
		// if we didn't find it, create one
		wmap = bmap_create();
		if(!wmap) {
			// if we couldn't, abort
			fprintf(stderr, "Couldn't create map to push to\n");
			return;
		}

		char pbuf[PBSIZE];
		sprintf(pbuf, "%p", (void *)wmap);
		// insert the location of wmap into ploc
		bmap_add(mkv->ploc, buf, pbuf);
	} else {
		// extract the location of wmap from ploc
		sscanf(m->val, "%p", (void **)&wmap);
	}

	// try to find the output in wmap
	BMap_Node *mn = bmap_find(wmap, words[mkv->order]);
	if(mn == NULL) {
		// if it's the first time, it has a value of 1
		bmap_add(wmap, words[mkv->order], "1");
	} else {
		// otherwise we should add 1 to the current value...
		int tmp = atoi(mn->val) + 1;
		char tmps[PBSIZE] = { 0 };
		sprintf(tmps, "%d", tmp);
		// and at it back in
		bmap_add(wmap, words[mkv->order], tmps);
	}
} // }}}

void markov_insert(Markov *mkv, char *str) { // {{{
	if(!mkv || !str)
		return;

	// reserve space for each word in order, plus the output
	char **words = malloc(sizeof(char *) * (mkv->order + 1));
	if(!words)
		return;
	for(int i = 0; i < mkv->order + 1; ++i) {
		words[i] = malloc(PBSIZE);
		if(!words[i]) {
			for(int j = 0; j < i; ++j)
				free(words[j]);
			free(words);
			return;
		}
		// clear everything
		memset(words[i], '\0', PBSIZE);
	}
	// start ends of words at smallest possible positions
	int so = 0, eo = 1;

	int wcnt;
	// get the initial set of words
	for(wcnt = 1; ; ++wcnt) {
		// skip over all spaces
		while(str[so] == ' ') so++, eo++;
		// if we just skipped to the end, break
		if(str[so] == '\0') {
			--wcnt;
			break;
		}

		// find end of word
		while((str[eo] != '\0') && (str[eo] != ' ')) eo++;

		// copy words one backward
		for(int i = 0; i < mkv->order; ++i)
			strcpy(words[i], words[i + 1]);

		// copy new word into words array
		strncpy(words[mkv->order], str + so, eo - so);
		words[mkv->order][eo - so] = '\0';

		if(wcnt >= mkv->order)
			// push current set of words
			markov_push(mkv, words);

		// no more words
		if(str[eo] == '\0')
			break;

		// advance to next possible start/end position
		so = eo;
		eo++;
	}

	if(wcnt >= mkv->order) {
		// copy words one backward
		for(int i = 0; i < mkv->order; ++i)
			strcpy(words[i], words[i + 1]);
		words[mkv->order][0] = '\0';
		markov_push(mkv, words);
	}

	for(int j = 0; j < mkv->order + 1; ++j)
		free(words[j]);
	free(words);
} // }}}

char *markov_search(Markov *mkv, char *str) {
	if(!mkv || !str)
		return NULL;

	return NULL;
}
