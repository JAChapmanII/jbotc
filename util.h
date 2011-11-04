#ifndef UTIL_H
#define UTIL_H

#include "lib/bmap.h"
#include <regex.h>

/* Returns a pointer to the string representation of a regular expression
 * error. Since this should be tested, this function should never be called in
 * production.
 */
char *getRegError(int errcode, regex_t *compiled);

/* Loads dumped variables from the dat file */
int readDump(BMap *readTo);

/* Loads dumped variables from the dat file */
int dumpVars(BMap *dumpFrom);

/* Opens log file for appending to */
int initLogFile();

/* Small wrapper to allow printing to a logFile and stdout at the same time */
void send(const char *target, char *format, ...);

/* Small wrapper to allow printing straight to logFile */
void lprintf(char *format, ...);

/* Flush the log */
void lflush();

#endif // UTIL_H
