#ifndef REGEX_H
#define REGEX_H

#include "arena.h"

/* a struct regex is just a struct dfa */
struct regex;

struct regex *regex_compile(struct arena *arena, char *str);

/* returns the length of a match starting from the beginning of str */
long regex_nongreedy_match(struct regex *regex, char *str);
long regex_greedy_match(struct regex *regex, char *str);

#endif
