#include "functions.h"

#include <stdio.h>
#include <string.h>

/* markov will eventually print markov chains generated from previous input. */
void markov(FunctionArgs *fa) { // {{{
	char *tok = strtok(fa->args, " ");
	if(tok == NULL) {
		// if we didn't recieve an argument, print usage
		send(fa->target, "%s: Usage: markov <word>", fa->name);
	} else {
		// we recieved an argument, print it back to them for now.
		// TODO: implement properly
		send(fa->target, "%s: %s", fa->name, tok);
	}
} // }}}

/* Declares variables to remember things. */
void declareVariable(FunctionArgs *fa) { // {{{
	BMap_Node *tmpn = NULL;

	char *tok = strtok(fa->args, " ");
	tmpn = bmap_find(fa->vars, tok);
	if(tmpn == NULL) {
		if(bmap_size(fa->vars) >= 256) {
			send(fa->target, "%s: 256 variables exist already, sorry!", fa->name);
		} else {
			bmap_add(fa->vars, tok, "0");
			send(fa->target, "%s: set \"%s\" to 0", fa->name, tok);
		}
	} else {
		send(fa->target, "%s: \"%s\" is %s", fa->name, tok, tmpn->val);
	}
} // }}}

/* Set a variable to remember things. */
void setVariable(FunctionArgs *fa) { // {{{
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
void incrementVariable(FunctionArgs *fa) { // {{{
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
void decrementVariable(FunctionArgs *fa) { // {{{
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

