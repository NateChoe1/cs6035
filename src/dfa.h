#ifndef DFA_H
#define DFA_H

#include <limits.h>

#include "arena.h"
#include "state.h"

struct dfa_node {
	/* generic value, its meaning depends on the specific dfa
	 * for example, for regexes, r is a boolean that tells you if this is an
	 * accept state */
	long r;

	/* `links` is an index in `struct dfa.nodes`
	 * len(links) == num_items */
	long *links;

	/* if save_states == 0, then this is always NULL */
	struct state *state;
};

struct dfa {
	/* invalid indexes (negative numbers and too large numbers) are always
	 * rejections */
	struct dfa_node *nodes;
	long num_nodes;
	long num_items;

	size_t alloc;
	struct arena *arena;
};

struct dfa_builder {
	/* converts state to closure(state) */
	void (*enclose)(struct state *state, void *arg);

	/* returns goto(state, item) */
	struct state *(*step)(struct arena *, struct state *, long item,
			void *arg);

	/* returns a set of items that could conceivably follow this
	 * state. `ret` is an array of chars of length `num_items`,
	 * where index 0 is a boolean indicating that item 0 could
	 * follow this state, index 1 for item 1, and so on. */
	void (*followups)(struct state *state, void *arg, char *ret);

	/* gets the `r` value, some generic long associated with each node
	 *
	 * this is used for example in regexes, where 1 indicates that this is
	 * an accept state.
	 *
	 * if this is NULL, then we default to f(s, arg) = NULL
	 * */
	long (*get_r)(struct state *state, void *arg);

	/* simplifies states such that any two states within the same
	 * equivalence class are equal to each other
	 *
	 * if this is NULL, then we default to the identity function */
	struct state *(*simplify)(struct arena *, struct state *, void *arg);
};

struct dfa *dfa_new(struct arena *arena, long num_items, int save_states,
		struct state *initial_state,
		struct dfa_builder *builder,

		/* generic argument passed to each of these functions as a
		 * closure */
		void *arg);

#endif
