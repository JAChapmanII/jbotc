#ifndef RLIST_H
#define RLISH_H

#include <regex.h>

typedef struct RList_Element {
	regex_t *r;
	char *data;
	char *regex;
	struct RList_Element *next;
} RList;

RList *rlist_create(void);
void rlist_free(RList *rlst);

char *rlist_add(RList *rlst, char *regex, char *data);
char *rlist_match(RList *rlst, char *msg);

#endif // RLIST_H
