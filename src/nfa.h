#ifndef NFA_H
#define NFA_H

#include <limits.h>

#include "arena.h"

#define NFA_CHARSET_SIZE 256

#define NFA_EMPTY_IDX ((int) NFA_CHARSET_SIZE + 1)
#define NFA_MAX_TRANSITIONS ((int) NFA_CHARSET_SIZE + 2)

struct nfa_list {
	long node;
	struct nfa_list *next;
};

struct nfa_node {
	int should_accept;
	struct nfa_list *transitions[NFA_MAX_TRANSITIONS];
};

struct nfa {
	struct arena *arena;

	size_t num_nodes;
	size_t alloc;
	long start_node;
	struct nfa_node *nodes;
};

struct nfa *nfa_compile(struct arena *arena, char *string);

#endif
