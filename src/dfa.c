#include "dfa.h"
#include "state.h"

/* returns the index of the new state */
static long add_state(struct dfa *dfa);

static void explore_state(struct dfa *dfa,
		struct arena *arena, int save_states,
		struct state *state, struct state_list *new_states,
		struct state_set *seen,
		void (*enclose)(struct state *state, void *arg),
		struct state *(*step)(struct arena *, struct state *,
			int item, void *arg),
		void (*get_followups)(struct state *state, void *arg, char *r),
		int (*get_r)(struct state *state, void *arg),
		void *arg,
		char *followups);

struct dfa *dfa_new(struct arena *arena, int num_items, int save_states,
		struct state *initial_state,
		void (*enclose)(struct state *state, void *arg),
		struct state *(*step)(struct arena *, struct state *,
			int item, void *arg),
		void (*followups)(struct state *state, void *arg, char *ret),
		int (*get_r)(struct state *state, void *arg),
		void *arg) {
	struct arena *ss, *as, *at1, *at2;
	struct state_set *state_set;
	struct state_list *unchecked, *new;
	struct dfa *ret;
	size_t i;
	char *fbuf;

	ret = arena_malloc(arena, sizeof(*ret));
	ret->num_nodes = 0;
	ret->alloc = 32;
	ret->num_items = num_items;
	ret->arena = arena;
	ret->nodes = arena_malloc(arena, ret->alloc * sizeof(*ret->nodes));

	as = save_states ? arena : arena_new();
	ss = arena_new();
	at1 = arena_new();

	state_set = state_set_new(ss);
	unchecked = state_list_new(at1);

	enclose(initial_state, arg);
	state_set_put(state_set, initial_state, add_state(ret));
	if (save_states) {
		ret->nodes[0].state = initial_state;
	}
	ret->nodes[0].r = get_r(initial_state, arg);
	state_list_add(unchecked, initial_state);

	fbuf = arena_malloc(ss, num_items);

	while (unchecked->len > 0) {
		at2 = arena_new();
		new = state_list_new(at2);

		for (i = 0; i < unchecked->len; ++i) {
			explore_state(ret, as, save_states,
					unchecked->states[i], new, state_set,
					enclose, step, followups, get_r, arg,
					fbuf);
		}

		arena_free(at1);
		at1 = at2;
		unchecked = new;
	}

	arena_free(at1);
	arena_free(ss);
	if (!save_states) {
		arena_free(as);
	}

	return ret;
}

static void explore_state(struct dfa *dfa,
		struct arena *arena, int save_states,
		struct state *state, struct state_list *new_states,
		struct state_set *seen,
		void (*enclose)(struct state *state, void *arg),
		struct state *(*step)(struct arena *, struct state *,
			int item, void *arg),
		void (*get_followups)(struct state *state, void *arg, char *r),
		int (*get_r)(struct state *state, void *arg),
		void *arg,
		char *followups) {
	struct state *new;
	int c;
	long old_state, new_state;

	old_state = state_set_get(seen, state);
	get_followups(state, arg, followups);

	for (c = 0; c < dfa->num_items; ++c) {
		if (followups[c] == 0) {
			continue;
		}

		new = step(arena, state, c, arg);
		enclose(new, arg);
		new_state = state_set_get(seen, new);
		if (new_state < 0) {
			goto make_new_state;
		}
		if (dfa->nodes[old_state].links[c] == -1) {
			goto existing_state;
		}
		continue;
make_new_state:
		new_state = add_state(dfa);
existing_state:
		state_list_add(new_states, new);
		dfa->nodes[new_state].r = get_r(new, arg);
		dfa->nodes[old_state].links[c] = new_state;
		if (save_states) {
			dfa->nodes[new_state].state = new;
		}
		state_set_put(seen, new, new_state);
	}
}

static long add_state(struct dfa *dfa) {
	int i;
	if (dfa->num_nodes >= (long) dfa->alloc) {
		dfa->alloc *= 2;
		dfa->nodes = arena_realloc(dfa->nodes,
				dfa->alloc * sizeof(*dfa->nodes));
	}

	dfa->nodes[dfa->num_nodes].r = 0;
	dfa->nodes[dfa->num_nodes].links = arena_malloc(dfa->arena,
			dfa->num_items *
				sizeof(*dfa->nodes[dfa->num_nodes].links));
	for (i = 0; i < dfa->num_items; ++i) {
		dfa->nodes[dfa->num_nodes].links[i] = -1;
	}

	return dfa->num_nodes++;
}
