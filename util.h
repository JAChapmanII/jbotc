#ifndef UTIL_H
#define UTIL_H

#include <regex.h>

#define BSIZE 4096
#define PBSIZE 256

/* Returns a pointer to the string representation of a regular expression
 * error. Since this should be tested, this function should never be called in
 * production.
 */
char *getRegError(int errcode, regex_t *compiled);

/* Opens log file for appending to */
int initLogFile();

/* Small wrapper to allow printing to a logFile and stdout at the same time */
void send(const char *target, char *format, ...);

/* Small wrapper to allow printing straight to logFile */
void lprintf(char *format, ...);

/* Flush the log */
void lflush();

#endif // UTIL_H
