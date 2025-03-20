#include "dfa.h"
#include "state.h"

/* returns the index of the new state */
static long add_state(struct dfa *dfa);

static void explore_state(struct dfa *dfa,
		struct arena *arena, struct arena *ss, int save_states,
		struct state *state, struct state_list *new_states,
		struct state_map *seen,
		struct dfa_builder *builder,
		void *arg,
		char *followups);

static long get_r_def(struct state *state, void *arg);
static struct state * simplify_def(struct arena *arena, struct state *state,
		void *arg);

struct dfa *dfa_new(struct arena *arena, long num_items, int save_states,
		struct state *initial_state,
		struct dfa_builder *builder,
		void *arg) {
	struct arena *ss, *as, *at1, *at2;
	struct state_map *state_map;
	struct state_list *unchecked, *new;
	struct state *simplified;
	struct dfa *ret;
	size_t i;
	char *fbuf;

	if (builder->get_r == NULL) {
		builder->get_r = get_r_def;
	}
	if (builder->simplify == NULL) {
		builder->simplify = simplify_def;
	}

	ret = arena_malloc(arena, sizeof(*ret));
	ret->num_nodes = 0;
	ret->alloc = 32;
	ret->num_items = num_items;
	ret->arena = arena;
	ret->nodes = arena_malloc(arena, ret->alloc * sizeof(*ret->nodes));

	as = save_states ? arena : arena_new();
	ss = arena_new();
	at1 = arena_new();

	state_map = state_map_new(ss);
	unchecked = state_list_new(at1);

	builder->enclose(initial_state, arg);
	simplified = builder->simplify(ss, initial_state, arg);
	state_map_put(state_map, simplified, add_state(ret));
	if (save_states) {
		ret->nodes[0].state = initial_state;
	}
	ret->nodes[0].r = builder->get_r(initial_state, arg);
	state_list_add(unchecked, initial_state);

	fbuf = arena_malloc(ss, num_items);

	while (unchecked->len > 0) {
		at2 = arena_new();
		new = state_list_new(at2);

		for (i = 0; i < unchecked->len; ++i) {
			explore_state(ret, as, ss, save_states,
					unchecked->states[i], new, state_map,
					builder, arg, fbuf);
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
		struct arena *arena, struct arena *ss, int save_states,
		struct state *state, struct state_list *new_states,
		struct state_map *seen,
		struct dfa_builder *builder,
		void *arg,
		char *followups) {
	struct state *simplified, *new;
	long c;
	long old_state, new_state;

	simplified = builder->simplify(ss, state, arg);
	old_state = state_map_get(seen, simplified);
	builder->followups(state, arg, followups);

	for (c = 0; c < dfa->num_items; ++c) {
		if (followups[c] == 0) {
			continue;
		}

		new = builder->step(arena, state, c, arg);
		builder->enclose(new, arg);
		simplified = builder->simplify(ss, new, arg);
		new_state = state_map_get(seen, simplified);
		if (new_state < 0) {
			goto make_new_state;
		}
		if (dfa->nodes[old_state].links[c] == -1) {
			goto existing_state;
		}
		continue;
make_new_state:
		new_state = add_state(dfa);
		state_list_add(new_states, new);
		state_map_put(seen, new, new_state);
		if (save_states) {
			dfa->nodes[new_state].state = new;
		}
		dfa->nodes[new_state].r = builder->get_r(new, arg);
existing_state:
		dfa->nodes[old_state].links[c] = new_state;
		if (save_states) {
			state_join(dfa->nodes[new_state].state, new);
		}
	}
}

static long add_state(struct dfa *dfa) {
	long i;
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

static long get_r_def(struct state *state, void *arg) {
	(void) state;
	(void) arg;
	return 0;
}

static struct state * simplify_def(struct arena *arena, struct state *state,
		void *arg) {
	(void) arena;
	(void) arg;
	return state;
}
