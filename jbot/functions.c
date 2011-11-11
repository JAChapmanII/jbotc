#include "functions.h"
#include "defines.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* Dislay the help  */
void help(FunctionArgs *fa) {
	send(fa->target, "%s: You're on your own.", fa->name);
}

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
	// initially tok will be "declare"...
	char *tok = strtok(fa->matchedOn, " ");
	// so we immediately strtok again
	tok = strtok(NULL, " ");

	BMap_Node *tmpn = bmap_find(fa->vars, tok);
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
	char *tok = strtok(fa->matchedOn, " ");
	// initially tok should be "set", if it isn't bail
	if(strcmp(tok, "set"))
		return;

	// we're now at the variable name
	char *var = strtok(NULL, " ");
	// next we want to move on to the value
	// TODO: if var is null, what happens here?
	char *val = strtok(NULL, " ");

	if(!var || !val) {
		send(fa->target, "%s: You must specify a variable and value", fa->name);
	} else {
		bmap_add(fa->vars, var, val);
		BMap_Node *tmpn = bmap_find(fa->vars, var);
		if(!tmpn) {
			send(fa->target, "%s: %s was not found!?", fa->name, var);
		} else {
			send(fa->target, "%s: %s set to %s", fa->name, var, tmpn->val);
		}
	}
} // }}}

/* Increment a variable and create it if it doesn't exist. */
void increment(FunctionArgs *fa) { // {{{
	char *var = NULL;
	// we matched a variable name in the regex, use it
	if(fa->matchCount == 1)
		var = fa->matchedOn + fa->matches[1].rm_so;
	else
		return; // something bad happened

	// If there is no variable or it is zero bytes long
	if(!var | !strlen(var)) {
		return;
	}

	BMap_Node *tmpn = bmap_find(fa->vars, var);
	if(tmpn == NULL) {
		if(bmap_size(fa->vars) >= 256) {
			send(fa->target, "%s: 256 variables exist already, sorry!", fa->name);
		} else {
			bmap_add(fa->vars, var, "0");
			send(fa->target, "%s: declared %s as 0", fa->name, var);
		}
	} else {
		char buf[PBSIZE];
		int val;

		// retrieve value, increment it
		val = atoi(tmpn->val);
		++val;

		// write the integer to the buffer
		snprintf(buf, PBSIZE, "%d", val);

		// save it back into the variable map
		bmap_add(fa->vars, var, buf);

		send(fa->target, "%s: %s incremented to %d", fa->name, var, val);
	}
} // }}}

/* Decrement a variable and create it if it doesn't exist. */
void decrement(FunctionArgs *fa) { // {{{
	char *var = NULL;
	// we matched a variable name in the regex, use it
	if(fa->matchCount == 1)
		var = fa->matchedOn + fa->matches[1].rm_so;
	else
		return; // something bad happened

	// If there is no variable or it is zero bytes long
	if(!var | !strlen(var)) {
		return;
	}

	BMap_Node *tmpn = bmap_find(fa->vars, var);
	if(tmpn == NULL) {
		if(bmap_size(fa->vars) >= 256) {
			send(fa->target, "%s: 256 variables exist already, sorry!", fa->name);
		} else {
			bmap_add(fa->vars, var, "0");
			send(fa->target, "%s: declared %s as 0", fa->name, var);
		}
	} else {
		char buf[PBSIZE];
		int val;

		// retrieve value, decrement it
		val = atoi(tmpn->val);
		--val;

		// write the integer to the buffer
		snprintf(buf, PBSIZE, "%d", val);

		// save it back into the variable map
		bmap_add(fa->vars, var, buf);

		send(fa->target, "%s: %s decremented to %d", fa->name, var, val);
	}
} // }}}

/* Answers an either/or question */
void eitherOr(FunctionArgs *fa) { // {{{
	if(!fa->toUs)
		return;
	if(rand() % 2)
		send(fa->target, "%s: %.*s", fa->name,
				fa->matches[1].rm_eo - fa->matches[1].rm_so,
				fa->matchedOn + fa->matches[1].rm_so);
	else
		send(fa->target, "%s: %.*s", fa->name,
				fa->matches[2].rm_eo - fa->matches[2].rm_so +
				// TODO: take care of this in regex?
				((fa->matchedOn[fa->matches[2].rm_eo - 1] == '?') ? -1 : 0),
				fa->matchedOn + fa->matches[2].rm_so);
} // }}}

void bmapn_inorder(BMap_Node *bmn, char *buf) {
	if(!bmn || !buf)
		return;
	bmapn_inorder(bmn->left, buf);
	strcat(buf, bmn->key);
	strcat(buf, ", ");
	bmapn_inorder(bmn->right, buf);
}

/* <3 */
void lessThanThree(FunctionArgs *fa) {
	send(fa->target, "%s: <3", fa->name);
}

/* List variables currently tracked */
void list(FunctionArgs *fa) { // {{{
	// TODO: constant here?
	char buf[PBSIZE * 256] = { 0 };
	bmapn_inorder(fa->vars->root, buf);
	// remove the trailing ", "
	buf[strlen(buf) - 2] = '\0';
	send(fa->target, "%s: %s", fa->name, buf);
} // }}}

