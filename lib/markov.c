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

void markov_push(Markov *mkv, char **words) {
	if(!mkv || !words)
		return;

	char buf[BSIZE] = { 0 };
	for(int i = 0; i < mkv->order; ++i) {
		strcat(buf, words[i]);
		if(i != mkv->order - 1)
			strcat(buf, " ");
	}

	BMap *wmap;
	BMap_Node *m = bmap_find(mkv->ploc, buf);

	if(m == NULL) {
		printf("\t");
		wmap = bmap_create();
		if(!wmap) {
			fprintf(stderr, "Couldn't create map to push to\n");
			printf("\n");
			return;
		}

		char pbuf[PBSIZE];
		sprintf(pbuf, "%p", wmap);
		bmap_add(mkv->ploc, buf, pbuf);
	} else {
		sscanf(m->val, "%p", (void **)&wmap);
	}

	printf("\"%s\" ", buf);
	printf("-> \"%s\"\n", words[mkv->order]);
}

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

