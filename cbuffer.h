#ifndef CBUFFER_H
#define CBUFFER_H

typedef struct {
	int size;
	char **lines;
	int push, pop;
} CBuffer;

CBuffer *cbuffer_construct(int size);
void cbuffer_free(CBuffer *cbuf);

char *cbuffer_pop(CBuffer *cbuf);
int cbuffer_push(CBuffer *cbuf, char *line);

#endif /* CBUFFER_H */
