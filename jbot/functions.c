#include "functions.h"
#include "defines.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* markov will eventually print markov chains generated from previous input. */
void markov(FunctionArgs *fa) { // {{{
	char *tok = strtok(fa->matchedOn, " ");
	// initially tok should be "markov", if it isn't bail
	if(strcmp(tok, "markov"))
		return;
	// if it is, we immediately strtok again
	tok = strtok(NULL, " ");
	if(tok == NULL) {
		// if we didn't recieve an argument, print usage
		send(fa->target, "%s: Usage: markov <word>", fa->name);
	} else {
		// we recieved an argument, print it back to them for now.
		// TODO: implement properly
		send(fa->target, "%s: %s", fa->name, tok);
	}
} // }}}

/* CodeBlock wants a fish... */
void fish(FunctionArgs *fa) { // {{{
	send(fa->target, "%s: %s", fa->name, ((rand() % 2) ? "><>" : "<><"));
} // }}}

/* CodeBlock wants multiple species of fish */
void fishes(FunctionArgs *fa) { // {{{
	char buf[80] = { 0 }, spaces[6] = "     ";
	int fcount = rand() % 5, scount = (rand() % 5) + 1;

	strcpy(buf, ((rand() % 2) ? "><>" : "<><"));
	strncat(buf, spaces, scount);

	for(int i = 0; i < fcount; ++i) {
		strcat(buf, ((rand() % 2) ? "><>" : "<><"));
		scount = (rand() % 5) + 1;
		strncat(buf, spaces, scount);
	}

	send(fa->target, "%s: %s", fa->name, buf);
} // }}}

/* WUB WUB WUB WUB WUB */
void dubstep(FunctionArgs *fa) { // {{{
	send(fa->target, "%s: WUB WUB WUB", fa->name);
} // }}}

/* Sidein wants a train! */
void sl(FunctionArgs *fa) { // {{{
	char buf[80] = { 0 };
	int ccount = rand() % 8;

	strcpy(buf, "/.==.]");
	for(int i = 0; i < ccount; ++i)
		strcat(buf, "[. .]");
	strcat(buf, "{. .}");

	send(fa->target, "%s: %s", fa->name, buf);
} // }}}

/* Wave if we haven't recently */
void wave(FunctionArgs *fa) { // {{{
	if(fa->toUs) {
		send(fa->target, "%s: %s", fa->name, ((rand() % 2) ? "o/" : "\\o"));
		return;
	}

	int doWave = 0;
	long curTime = time(NULL);

	BMap_Node *tmpn = NULL;
	tmpn = bmap_find(fa->vars, "__last_wave_time");
	if(tmpn == NULL) {
		doWave = 1;
		bmap_add(fa->vars, "__last_wave_time", "0");
	} else {
		int tmp = atoi(tmpn->val);
		if(curTime - tmp > 60)
			doWave = 1;
	}

	if(doWave) {
		char buf[PBSIZE];

		snprintf(buf, PBSIZE, "%ld", curTime);
		bmap_add(fa->vars, "__last_wave_time", buf);
		send(fa->target, "%s", ((rand() % 2) ? "o/" : "\\o"));
	}
} // }}}

/* Declares variables to remember things. */
void declare(FunctionArgs *fa) { // {{{
	// TODO: this can only declare "declare" and will return its value...
	BMap_Node *tmpn = NULL;

	// initially tok will be "declare"...
	char *tok = strtok(fa->matchedOn, " ");
	// so we immediately strtok again
	tok = strtok(NULL, " ");
	tmpn = bmap_find(fa->vars, tok);
	if(tmpn == NULL) {
		if(bmap_size(fa->vars) >= 256) {
			send(fa->target, "%s: 256 variables exist already, sorry!", fa->name);
		} else {
			bmap_add(fa->vars, tok, "0");
			send(fa->target, "%s: declared %s as 0", fa->name, tok);
		}
	} else {
		send(fa->target, "%s: %s is %s", fa->name, tok, tmpn->val);
	}
} // }}}

/* Set a variable to remember things. */
void set(FunctionArgs *fa) { // {{{
	BMap_Node *tmpn = NULL;
	char *tmpsp;

	char *tok = strtok(NULL, " ");
	tmpsp = tok;
	tok = strtok(NULL, " ");
	if(!tok || !tmpsp) {
		send(fa->target, "%s: You must specify a variable and value", fa->name);
	} else {
		bmap_add(fa->vars, tmpsp, tok);
		tmpn = bmap_find(fa->vars, tmpsp);
		if(!tmpn) {
			send(fa->target, "%s: \"%s\" was not found!?", fa->name, tmpsp);
		} else {
			send(fa->target, "%s: \"%s\" is %s", fa->name, tmpsp, tmpn->val);
		}
	}
} // }}}

/* Increment a variable and create it if it doesn't exist. */
void increment(FunctionArgs *fa) { // {{{
	BMap_Node *tmpn = NULL;

	char *tok = strtok(NULL, " ");
	tmpn = bmap_find(fa->vars, tok);
	if(tmpn == NULL) {
		if(bmap_size(fa->vars) >= 256) {
			send(fa->target, "%s: 256 variables exist already, sorry!", fa->name);
		} else {
			bmap_add(fa->vars, tok, "0");
			send(fa->target, "%s: set \"%s\" to 0", fa->name, tok);
		}
	} else {
		int tmp;
		char tmps[PBSIZE];

		tmp = atoi(tmpn->val);
		++tmp;
		snprintf(tmps, PBSIZE, "%d", tmp);
		bmap_add(fa->vars, tok, tmps);
		send(fa->target, "%s: \"%s\" is %d", fa->name, tok, tmp);
	}
} // }}}

/* Decrement a variable and create it if it doesn't exist. */
void decrement(FunctionArgs *fa) { // {{{
	BMap_Node *tmpn = NULL;

	char *tok = strtok(NULL, " ");
	tmpn = bmap_find(fa->vars, tok);
	if(tmpn == NULL) {
		if(bmap_size(fa->vars) >= 256) {
			send(fa->target, "%s: 256 variables exist already, sorry!", fa->name);
		} else {
			bmap_add(fa->vars, tok, "0");
			send(fa->target, "%s: set \"%s\" to 0", fa->name, tok);
		}
	} else {
		int tmp;
		char tmps[PBSIZE];

		tmp = atoi(tmpn->val);
		tmp--;
		snprintf(tmps, PBSIZE, "%d", tmp);
		bmap_add(fa->vars, tok, tmps);
		send(fa->target, "%s: \"%s\" is %d", fa->name, tok, tmp);
	}
} // }}}

