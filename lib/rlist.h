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

int rlist_size(RList *rlst);

char *rlist_add(RList *rlst, char *regex, char *data);
int rlist_remove(RList *rlst, char *regex);
char *rlist_match(RList *rlst, char *msg);

/* Dump the RList to a file */
int rlist_dump(RList *rlst, char *fileName);
/* Read a RList from a file */
int rlist_read(RList *rlst, char *fileName);

#endif // RLIST_H
