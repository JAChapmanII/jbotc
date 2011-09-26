#ifndef CBUFFER_H
#define CBUFFER_H

/* CBuffer is a structure representing a circular buffer, which aims to be
 * able to be read from in one thread, and written to in another.
 *
 * CBuffer owns the data that is pushed to it, and disowns the data popped
 * from it. Note: TODO this may change
 */
typedef struct {
	int size;
	char **lines;
	int push, pop;
} CBuffer;

/* Create a new CBuffer object with room for size lines in it
 *
 * Returns a pointer to a CBuffer on success, or
 * 	NULL on failure (does not leak)
 */
CBuffer *cbuffer_create(int size);
/* Frees all memory associated with a CBuffer object
 */
void cbuffer_free(CBuffer *cbuf);

/* Pops a line from a CBuffer
 *
 * Returns a char * on success, or
 * 	NULL if there is nothing to pop
 */
char *cbuffer_pop(CBuffer *cbuf);
/* Pushes a line into the CBuffer
 *
 * Returns true on success, or
 * 	false on failure (buffer is full)
 */
int cbuffer_push(CBuffer *cbuf, char *line);

#endif /* CBUFFER_H */
