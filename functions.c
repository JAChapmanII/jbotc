#include "functions.h"

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

	tok = strtok(NULL, " ");
	tmpsp = tok;
	tok = strtok(NULL, " ");
	if(!tok || !tmpsp) {
		send(chan, "%s: You must specify a variable and value", name);
	} else {
		bmap_add(variableMap, tmpsp, tok);
		tmpn = bmap_find(variableMap, tmpsp);
		if(!tmpn) {
			send(chan, "%s: \"%s\" was not found!?", name, tmpsp);
		} else {
			send(chan, "%s: \"%s\" is %s", name, tmpsp, tmpn->val);
		}
	}
} // }}}

/* Increment a variable and create it if it doesn't exist. */
void incrementVariable(FunctionArgs *fa) { // {{{
	BMap_Node *tmpn = NULL;

	tok = strtok(NULL, " ");
	tmpn = bmap_find(variableMap, tok);
	if(tmpn == NULL) {
		if(bmap_size(variableMap) >= 256) {
			send(chan, "%s: 256 variables exist already, sorry!", name);
		} else {
			bmap_add(variableMap, tok, "0");
			send(chan, "%s: set \"%s\" to 0", name, tok);
		}
	} else {
		int tmp;
		char tmps[PBSIZE];

		tmp = atoi(tmpn->val);
		++tmp;
		snprintf(tmps, PBSIZE, "%d", tmp);
		bmap_add(variableMap, tok, tmps);
		send(chan, "%s: \"%s\" is %d", name, tok, tmp);
	}
} // }}}

/* Decrement a variable and create it if it doesn't exist. */
void decrementVariable(FunctionArgs *fa) { // {{{
	BMap_Node *tmpn = NULL;

	tok = strtok(NULL, " ");
	tmpn = bmap_find(variableMap, tok);
	if(tmpn == NULL) {
		if(bmap_size(variableMap) >= 256) {
			send(chan, "%s: 256 variables exist already, sorry!", name);
		} else {
			bmap_add(variableMap, tok, "0");
			send(chan, "%s: set \"%s\" to 0", name, tok);
		}
	} else {
		int tmp;
		char tmps[PBSIZE];

		tmp = atoi(tmpn->val);
		tmp--;
		snprintf(tmps, PBSIZE, "%d", tmp);
		bmap_add(variableMap, tok, tmps);
		send(chan, "%s: \"%s\" is %d", name, tok, tmp);
	}
} // }}}

