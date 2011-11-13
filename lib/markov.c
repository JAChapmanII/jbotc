#include "markov.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

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

int bmapn_sum(BMap_Node *bmn) { // {{{
	if(!bmn)
		return 0;

	return atoi(bmn->val) + bmapn_sum(bmn->left) + bmapn_sum(bmn->right);
} // }}}
int bmap_sum(BMap *bmap) { // {{{
	if(!bmap)
		return 0;
	return bmapn_sum(bmap->root);
} // }}}

char *bmapn_keyAfterValue(BMap_Node *bmn, int val) { // {{{
	if(!bmn)
		return NULL;

	char *tmp = bmapn_keyAfterValue(bmn->left, val);
	if(tmp)
		return tmp;

	val -= bmapn_sum(bmn->left) + atoi(bmn->val);
	if(val < 0)
		return bmn->key;

	return bmapn_keyAfterValue(bmn->right, val);
} // }}}
char *bmap_keyAfterValue(BMap *bmap, int val) { // {{{
	if(!bmap)
		return NULL;
	return bmapn_keyAfterValue(bmap->root, val);
} // }}}

char *markov_search(Markov *mkv, char *str) { // {{{
	if(!mkv || !str)
		return NULL;

	BMap *wmap;
	BMap_Node *m = bmap_find(mkv->ploc, str);

	// if the entry doesn't exist, bail
	if(!m)
		return NULL;

	// extract the location of wmap from ploc
	sscanf(m->val, "%p", (void **)&wmap);

	int sum = bmap_sum(wmap);
	int entry = rand() % sum;

	return bmap_keyAfterValue(wmap, entry);
} // }}}
char *markov_fetch(Markov *mkv, char *seed, int maxLength) { // {{{
	if(!mkv || !seed || (maxLength < 1))
		return NULL;
	if(strlen(seed) >= (unsigned)maxLength)
		return NULL;

	char *buf = malloc(maxLength);
	if(!buf)
		return NULL;
	memset(buf, '\0', maxLength);

	int length = strlen(seed) + 1;
	strcpy(buf, seed);
	strcat(buf, " ");

	char *word = markov_search(mkv, seed);
	int so = 0;
	while((length < maxLength) && (word != NULL) && strlen(word)) {
		int rlen = strlen(word);
		if(rlen + length >= maxLength)
			break;
		strcat(buf, word);
		length += rlen;

		while(buf[so] != ' ')
			so++;
		so++;

		word = markov_search(mkv, buf + so);

		strcat(buf, " ");
		length++;
	}
	buf[length - 1] = '\0';
	return buf;
} // }}}

int markov_ensureDirectory(char *dumpDirectory) { // {{{
	if(!dumpDirectory)
		return 0;
	int status = mkdir(dumpDirectory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if(!status)
		return 1;
	if(errno == EEXIST)
		return 1;
	return 0;
} // }}}

int markov_read_ploc(Markov *mkv, BMap_Node *bmn, char *directoryName) {
	if(!mkv || !bmn || !directoryName)
		return 0;

	BMap *p = bmap_create();
	if(!p)
		return 0;

	char buf[BSIZE];
	strcpy(buf, directoryName);
	strcat(buf, "/");
	strcat(buf, bmn->key);

	bmap_read(p, buf);

	sprintf(buf, "%p", (void *)p);
	bmap_add(mkv->ploc, bmn->key, buf);

	markov_read_ploc(mkv, bmn->left, directoryName);
	markov_read_ploc(mkv, bmn->right, directoryName);
	return 1;
}
int markov_read(Markov *mkv, char *fileName, char *directoryName) {
	if(!mkv || !fileName || !directoryName)
		return 0;

	if(!markov_ensureDirectory(directoryName)) {
		fprintf(stderr, "Could not ensure %s exists!\n", directoryName);
		return 0;
	}

	int count = bmap_read(mkv->ploc, fileName);
	printf("read %d entries\n", count);
	if(!count)
		return 0;

	markov_read_ploc(mkv, mkv->ploc->root, directoryName);
	return count;
}

int markov_dump_ploc(BMap_Node *bmn, char *directoryName) { // {{{
	if(!bmn)
		return 0;

	char buf[BSIZE];
	strcpy(buf, directoryName);
	strcat(buf, "/");
	strcat(buf, bmn->key);

	BMap *p;
	sscanf(bmn->val, "%p", (void **)&p);
	int count = bmap_dump(p, buf);

	count += markov_dump_ploc(bmn->left, directoryName);
	count += markov_dump_ploc(bmn->right, directoryName);
	return count;
} // }}}
int markov_dump(Markov *mkv, char *fileName, char *directoryName) { // {{{
	if(!mkv || !fileName || !directoryName)
		return 0;

	if(!markov_ensureDirectory(directoryName)) {
		fprintf(stderr, "Could not ensure %s exists!\n", directoryName);
		return 0;
	}

	int count = bmap_dump(mkv->ploc, fileName);
	printf("dumped %d entries\n", count);
	if(!count)
		return 0;

	return markov_dump_ploc(mkv->ploc->root, directoryName);
} // }}}

