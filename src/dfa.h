#ifndef DFA_H
#define DFA_H

#include <limits.h>

#include "arena.h"

struct dfa_node {
	int accept;

	/* `links` is an index in `struct dfa.nodes` */
	int links[UCHAR_MAX];
};

struct dfa {
	/* invalid indexes (negative numbers and too large numbers) are always
	 * rejections */
	struct dfa_node *nodes;
	long num_nodes;

	size_t alloc;
};

struct dfa *dfa_new(struct arena *arena, char *regex);

/* returns the length of the shortest accepting prefix in str, or -1 if there is
 * none */
long dfa_check_loose(struct dfa *dfa, char *str);

/* returns the length of the longest accepting prefix in str, or -1 if there is
 * none */
long dfa_check_greedy(struct dfa *dfa, char *str);

#endif
