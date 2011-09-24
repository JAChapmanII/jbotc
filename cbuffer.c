#include "cbuffer.h"
#include <stdlib.h>

CBuffer *cbuffer_create(int size) {
	CBuffer *cbuf = malloc(sizeof(CBuffer));
	if(!cbuf)
		return NULL;
	cbuf->size = size;
	cbuf->lines = calloc(sizeof(char *), cbuf->size);
	if(!cbuf->lines) {
		cbuffer_free(cbuf);
		return NULL;
	}
	cbuf->push = cbuf->pop = 0;
	return cbuf;
}

void cbuffer_free(CBuffer *cbuf) {
	int i;
	if(!cbuf)
		return;
	for(i = 0; i < cbuf->size; ++i)
		free(cbuf->lines[i]);
	free(cbuf->lines);
	free(cbuf);
}

char *cbuffer_pop(CBuffer *cbuf) {
	char *tmp;
	cbuf->pop %= cbuf->size;
	if((tmp = cbuf->lines[cbuf->pop]) == NULL)
		return NULL;
	cbuf->lines[cbuf->pop++] = NULL;
	return tmp;
}

int cbuffer_push(CBuffer *cbuf, char *line) {
	cbuf->push %= cbuf->size;
	if(cbuf->lines[cbuf->push] != NULL)
		return 0;
	cbuf->lines[cbuf->push++] = line;
	return 1;
}

