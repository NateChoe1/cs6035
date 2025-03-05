#ifndef DFA_H
#define DFA_H

#include <limits.h>

#include "arena.h"
#include "state.h"

struct dfa_node {
	/* generic value, its meaning depends on the specific dfa
	 * for example, for regexes, r is a boolean that tells you if this is an
	 * accept state */
	int r;

	/* `links` is an index in `struct dfa.nodes`
	 * len(links) == num_items */
	int *links;
};

struct dfa {
	/* invalid indexes (negative numbers and too large numbers) are always
	 * rejections */
	struct dfa_node *nodes;
	long num_nodes;
	int num_items;

	size_t alloc;
	struct arena *arena;
};

struct dfa *dfa_new(struct arena *arena, int num_items,
		/* initial state of the dfa, doesn't have to be a closure, may
		 * be modified by this function, should be discarded with an
		 * arena after use */
		struct state *initial_state,

		/* converts state to closure(state) */
		void (*enclose)(struct state *state, void *arg),

		/* returns goto(state, item) */
		struct state *(*step)(struct arena *, struct state *,
			int item, void *arg),

		/* returns a set of items that could conceivably follow this
		 * state. return value is an array of chars of length
		 * `num_items`, where index 0 is a boolean indicating that item
		 * 0 could follow this state, index 1 for item 1, and so on. */
		char *(*followups)(struct state *state, void *arg),

		/* gets the `r` value, some generic integer */
		int (*get_r)(struct state *state, void *arg),

		/* generic argument passed to each of these functions for state
		 * management */
		void *arg);

#endif
