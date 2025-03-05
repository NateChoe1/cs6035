#include "dfa.h"
#include "state.h"
#include "hashset.h"

/* returns the index of the new state */
static long add_state(struct dfa *dfa);

static void dfa_callback(void *closure_raw, void *item_raw);

struct dfa_closure {
	struct arena *arena;
	struct state_set *state_set;
	struct dfa *dfa;
	struct hashset *new;
	void (*enclose)(struct state *state, void *arg);
	struct state *(*step)(struct arena *, struct state *,
			int item, void *arg);
	char *(*followups)(struct state *state, void *arg);
	int (*get_r)(struct state *state, void *arg);
	void *arg;
};

struct dfa *dfa_new(struct arena *arena, int num_items,
		struct state *initial_state,
		void (*enclose)(struct state *state, void *arg),
		struct state *(*step)(struct arena *, struct state *,
			int item, void *arg),
		char *(*followups)(struct state *state, void *arg),
		int (*get_r)(struct state *state, void *arg),
		void *arg) {
	struct arena *as, *at1, *at2;
	struct state_set *state_set;
	struct hashset *unchecked, *new;
	struct dfa_closure closure;
	struct dfa *ret;

	ret = arena_malloc(arena, sizeof(*ret));
	ret->num_nodes = 0;
	ret->alloc = 32;
	ret->num_items = num_items;
	ret->arena = arena;
	ret->nodes = arena_malloc(arena, ret->alloc * sizeof(*ret->nodes));

	as = arena_new();
	at1 = arena_new();

	state_set = state_set_new(as);
	unchecked = hashset_new(at1);

	enclose(initial_state, arg);
	state_set_put(state_set, initial_state, add_state(ret));
	ret->nodes[0].r = get_r(initial_state, arg);
	hashset_put(unchecked, initial_state);

	closure.arena = as;
	closure.state_set = state_set;
	closure.dfa = ret;
	closure.enclose = enclose;
	closure.step = step;
	closure.followups = followups;
	closure.get_r = get_r;
	closure.arg = arg;
	while (hashset_size(unchecked) > 0) {
		at2 = arena_new();
		new = hashset_new(at2);
		closure.new = new;
		hashset_iter(unchecked, &closure, dfa_callback);
		arena_free(at1);
		at1 = at2;
		unchecked = new;
	}

	arena_free(at1);
	arena_free(as);

	return ret;
}

static void dfa_callback(void *closure_raw, void *item_raw) {
	struct dfa_closure *closure;
	struct state *item, *new;
	int c, r;
	long old_state, new_state;
	char *followups;

	closure = (struct dfa_closure *) closure_raw;
	item = (struct state *) item_raw;
	old_state = state_set_get(closure->state_set, item);
	followups = closure->followups(item, closure->arg);

	for (c = 0; c < closure->dfa->num_items; ++c) {
		if (followups[c] == 0) {
			continue;
		}

		new = closure->step(closure->arena, item, c, closure->arg);
		closure->enclose(new, closure->arg);
		new_state = state_set_get(closure->state_set, new);
		if (new_state < 0) {
			goto make_new_state;
		}
		if (closure->dfa->nodes[old_state].links[c] == -1) {
			goto existing_state;
		}
		continue;
make_new_state:
		new_state = add_state(closure->dfa);
existing_state:
		hashset_put(closure->new, new);
		r = closure->get_r(new, closure->arg);
		closure->dfa->nodes[new_state].r = r;
		closure->dfa->nodes[old_state].links[c] = new_state;
		state_set_put(closure->state_set, new, new_state);
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
	for (i = 0; i < UCHAR_MAX; ++i) {
		dfa->nodes[dfa->num_nodes].links[i] = -1;
	}

	return dfa->num_nodes++;
}
