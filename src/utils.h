#ifndef UTILS_H
#define UTILS_H

int startswith(char *string, char *prefix);

#define ERROR "[\x1b[41;30;1mERROR\x1b[0m]\t"
#define INFO "[\x1b[33;1mINFO\x1b[0m]\t"
void report(char *level, char *message, ...);

#endif
