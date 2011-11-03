#ifndef UTIL_H
#define UTIL_H

/* Returns a random greeting from greetings array */
const char *obtainGreeting();

/* Returns a pointer to the string representation of a regular expression
 * error. Since this should be tested, this function should never be called in
 * production.
 */
char *getRegError(int errcode, regex_t *compiled);

/* Opens log file for appending to */
int initLogFile();

/* Small wrapper to allow printing to a logFile and stdout at the same time */
void send(const char *target, char *format, ...);

#endif // UTIL_H
