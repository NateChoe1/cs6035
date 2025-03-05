#include "dfa.h"
#include "nfa.h"
#include "state.h"
#include "hashset.h"

static void convert(struct dfa *dfa, struct nfa_node *nfa);
static void convert_callback(void *closure_raw, void *item_raw);

/* converts state to closure(state) */
static void enclose(struct state *state);
static void enclose_callback(void *closure_raw, void *item);

static struct state *transition(struct arena *arena,
		struct state *state, unsigned char c);

/* returns the index of the new state */
static long add_state(struct dfa *dfa);

static int state_accepted(struct state *state);

struct dfa *dfa_new(struct arena *arena, char *regex) {
	struct arena *tmp_arena;
	struct nfa_node *nfa;
	struct dfa *ret;

	tmp_arena = arena_new();
	nfa = nfa_compile(tmp_arena, regex);

	ret = arena_malloc(arena, sizeof(*ret));
	ret->num_nodes = 0;
	ret->alloc = 32;
	ret->nodes = arena_malloc(arena, ret->alloc * sizeof(*ret->nodes));
	convert(ret, nfa);

	arena_free(tmp_arena);

	return ret;
}

long dfa_check_loose(struct dfa *dfa, char *str) {
	long i, state;

	if (dfa->nodes[0].accept) {
		return 0;
	}

	state = 0;
	for (i = 0; str[i]; ++i) {
		if (state < 0 || state >= dfa->num_nodes) {
			return -1;
		}
		state = dfa->nodes[state].links[(unsigned char) str[i]];
		if (dfa->nodes[state].accept) {
			return i+1;
		}
	}
	return -1;
}

long dfa_check_greedy(struct dfa *dfa, char *str) {
	long r, i, state;

	r = dfa->nodes[0].accept ? 0 : -1;

	state = 0;
	for (i = 0; str[i]; ++i) {
		if (state < 0 || state >= dfa->num_nodes) {
			break;
		}
		state = dfa->nodes[state].links[(unsigned char) str[i]];
		if (dfa->nodes[state].accept) {
			r = i+1;
		}
	}
	return r;
}

struct convert_closure {
	struct arena *arena;
	struct state_set *state_set;
	struct dfa *dfa;
	struct hashset *new;
};

static void convert(struct dfa *dfa, struct nfa_node *nfa) {
	struct arena *arena, *at1, *at2;
	struct state_set *state_set;
	struct state *initial_state;
	struct hashset *unchecked, *new;
	struct convert_closure closure;

	arena = arena_new();
	at1 = arena_new();

	state_set = state_set_new(arena);
	unchecked = hashset_new(at1);

	initial_state = state_new(arena);
	state_append(initial_state, nfa);
	enclose(initial_state);
	state_set_put(state_set, initial_state, add_state(dfa));
	if (state_accepted(initial_state)) {
		dfa->nodes[0].accept = 1;
	}
	hashset_put(unchecked, initial_state);

	closure.arena = arena;
	closure.state_set = state_set;
	closure.dfa = dfa;
	while (hashset_size(unchecked) > 0) {
		at2 = arena_new();
		new = hashset_new(at2);

		closure.new = new;
		hashset_iter(unchecked, &closure, convert_callback);

		arena_free(at1);
		at1 = at2;
		unchecked = new;
	}

	arena_free(at1);
	arena_free(arena);
}

#include <stdio.h>
static void convert_callback(void *closure_raw, void *item_raw) {
	struct convert_closure *closure;
	struct state *item, *new;
	int c, accepted;
	long old_state, new_state;

	closure = (struct convert_closure *) closure_raw;
	item = (struct state *) item_raw;
	old_state = state_set_get(closure->state_set, item);

	for (c = 0; c < UCHAR_MAX; ++c) {
		new = transition(closure->arena, item, c);
		enclose(new);
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
		accepted = state_accepted(new);
		closure->dfa->nodes[new_state].accept = accepted;
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

	dfa->nodes[dfa->num_nodes].accept = 0;
	for (i = 0; i < UCHAR_MAX; ++i) {
		dfa->nodes[dfa->num_nodes].links[i] = -1;
	}

	return dfa->num_nodes++;
}

struct enclose_closure {
	struct hashset *added;
	struct state *state;
	struct hashset *new;
};

static void enclose(struct state *state) {
	struct hashset *added, *unchecked, *new;
	struct state_item *iter;
	struct arena *pm_arena, *tmp_arena, *tmp2_arena;
	struct enclose_closure closure;

	pm_arena = arena_new();
	tmp_arena = arena_new();

	added = hashset_new(pm_arena);
	unchecked = hashset_new(tmp_arena);
	iter = state->head;
	while (iter != NULL) {
		hashset_put(added, iter->value);
		hashset_put(unchecked, iter->value);
		iter = iter->next;
	}

	closure.added = added;
	closure.state = state;
	while (hashset_size(unchecked) > 0) {
		tmp2_arena = arena_new();
		new = hashset_new(tmp2_arena);

		closure.new = new;
		hashset_iter(unchecked, &closure, enclose_callback);

		arena_free(tmp_arena);
		unchecked = new;
		tmp_arena = tmp2_arena;
	}

	arena_free(pm_arena);
	arena_free(tmp_arena);
}

static void enclose_callback(void *closure_raw, void *item_raw) {
	struct enclose_closure *closure;
	struct nfa_node *item;
	struct nfa_list *iter;
	closure = closure_raw;
	item = item_raw;
	iter = item->transitions[NFA_EMPTY_IDX];
	while (iter != NULL) {
		if (!hashset_contains(closure->added, iter->node)) {
			hashset_put(closure->added, iter->node);
			state_append(closure->state, iter->node);
			hashset_put(closure->new, iter->node);
		}
		iter = iter->next;
	}
}

static struct state *transition(struct arena *arena,
		struct state *state, unsigned char c) {
	struct state *ret;
	struct state_item *state_iter;
	struct nfa_list *nfa_iter;
	struct nfa_node *node;

	ret = state_new(arena);
	state_iter = state->head;
	while (state_iter != NULL) {
		node = state_iter->value;
		nfa_iter = node->transitions[c];
		while (nfa_iter != NULL) {
			state_append(ret, nfa_iter->node);
			nfa_iter = nfa_iter->next;
		}
		state_iter = state_iter->next;
	}
	return ret;
}

static int state_accepted(struct state *state) {
	struct state_item *iter;
	struct nfa_node *node;
	iter = state->head;
	while (iter != NULL) {
		node = (struct nfa_node *) iter->value;
		if (node->should_accept) {
			return 1;
		}
		iter = iter->next;
	}
	return 0;
}
