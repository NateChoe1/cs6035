#include "dfa.h"
#include "nfa.h"
#include "regex.h"
#include "hashset.h"

#include <string.h>
#include <limits.h>

/* converts state to closure(state) */
static void enclose(struct state *state, void *arg);
static void enclose_item(struct nfa *nfa,
		struct state *state, struct state_ordered *new,
		struct hashset *added, long start);

static struct state *transition(struct arena *arena,
		struct state *state, int c, void *arg);

static char *followups(struct state *state, void *arg);

static int state_accepted(struct state *state, void *arg);

struct regex *regex_compile(struct arena *arena, char *pattern) {
	struct dfa *ret;
	struct arena *ta;
	struct nfa *nfa;
	struct state *initial_state;

	ta = arena_new();
	nfa = nfa_compile(ta, pattern);
	initial_state = state_new(arena);
	state_append(initial_state, nfa->start_node);

	ret = dfa_new(arena, NFA_MAX_TRANSITIONS, initial_state,
			enclose,
			transition,
			followups,
			state_accepted,
			nfa);

	arena_free(ta);

	return (struct regex *) ret;
}

long regex_nongreedy_match(struct regex *regex, char *str) {
	struct dfa *dfa;
	long i, state;

	dfa = (struct dfa *) regex;

	if (dfa->nodes[0].r) {
		return 0;
	}

	state = 0;
	for (i = 0; str[i]; ++i) {
		if (state < 0 || state >= dfa->num_nodes) {
			return -1;
		}
		state = dfa->nodes[state].links[(unsigned char) str[i]];
		if (dfa->nodes[state].r) {
			return i+1;
		}
	}
	return -1;
}

long regex_greedy_match(struct regex *regex, char *str) {
	struct dfa *dfa;
	long r, i, state;

	dfa = (struct dfa *) regex;

	r = dfa->nodes[0].r ? 0 : -1;

	state = 0;
	for (i = 0; str[i]; ++i) {
		if (state < 0 || state >= dfa->num_nodes) {
			break;
		}
		state = dfa->nodes[state].links[(unsigned char) str[i]];
		if (dfa->nodes[state].r) {
			r = i+1;
		}
	}
	return r;
}

struct enclose_closure {
	struct nfa *nfa;
	struct hashset *added;
	struct state *state;
	struct hashset *new;
};

static void enclose(struct state *state, void *arg) {
	struct hashset *added;
	struct state_ordered *unchecked, *new;
	struct state_item *iter;
	struct arena *pm_arena, *tmp_arena, *tmp2_arena;
	struct nfa *nfa;
	size_t i;

	nfa = (struct nfa *) arg;

	pm_arena = arena_new();
	tmp_arena = arena_new();

	added = hashset_new(pm_arena);
	unchecked = state_ordered_new(tmp_arena);
	iter = state->head;
	while (iter != NULL) {
		hashset_put(added, iter->value);
		state_ordered_put(unchecked, iter->value);
		iter = iter->next;
	}

	while (unchecked->size > 0) {
		tmp2_arena = arena_new();
		new = state_ordered_new(tmp2_arena);

		for (i = 0; i < unchecked->size; ++i) {
			enclose_item(nfa, state, new,
					added, unchecked->items[i]);
		}

		arena_free(tmp_arena);
		unchecked = new;
		tmp_arena = tmp2_arena;
	}

	arena_free(pm_arena);
	arena_free(tmp_arena);
}

static void enclose_item(struct nfa *nfa,
		struct state *state, struct state_ordered *new,
		struct hashset *added, long start) {
	struct nfa_list *iter;
	iter = nfa->nodes[start].transitions[NFA_EMPTY_IDX];
	while (iter != NULL) {
		if (!hashset_contains(added, iter->node)) {
			hashset_put(added, iter->node);
			state_append(state, iter->node);
			state_ordered_put(new, iter->node);
		}
		iter = iter->next;
	}
}

static struct state *transition(struct arena *arena,
		struct state *state, int c, void *arg) {
	struct state *ret;
	struct state_item *state_iter;
	struct nfa_list *nfa_iter;
	long node;
	struct nfa *nfa;

	nfa = (struct nfa *) arg;

	ret = state_new(arena);
	state_iter = state->head;
	while (state_iter != NULL) {
		node = state_iter->value;
		nfa_iter = nfa->nodes[node].transitions[c];
		while (nfa_iter != NULL) {
			state_append(ret, nfa_iter->node);
			nfa_iter = nfa_iter->next;
		}
		state_iter = state_iter->next;
	}
	return ret;
}

/* TODO: smarter followups function */
static char *followups(struct state *state, void *arg) {
	static char ret[NFA_MAX_TRANSITIONS] = {0};
	(void) state;
	(void) arg;

	if (ret[0] == '\0') {
		memset(ret, 1, sizeof(ret));
	}
	return ret;
}

static int state_accepted(struct state *state, void *arg) {
	struct state_item *iter;
	struct nfa *nfa;

	nfa = (struct nfa *) arg;

	iter = state->head;
	while (iter != NULL) {
		if (nfa->nodes[iter->value].should_accept) {
			return 1;
		}
		iter = iter->next;
	}
	return 0;
}
