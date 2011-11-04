#include "greetings.h"
#include <stdlib.h>

#define GREETING_COUNT 5
char *greetings[GREETING_COUNT] = {
	"Ahoy!", "Howdy!", "Goodday!", "Hello!", "Kill all humans."
};

/* Returns a random greeting from greetings array */
const char *obtainGreeting() { // {{{
	return greetings[rand() % GREETING_COUNT];
} // }}}

