#include "dfa.h"
#include "nfa.h"
#include "regex.h"
#include "hashset.h"

#include <string.h>
#include <limits.h>

/* converts state to closure(state) */
static void enclose(struct state *state, void *arg);
static void enclose_callback(void *closure_raw, void *item);

static struct state *transition(struct arena *arena,
		struct state *state, int c, void *arg);

static char *followups(struct state *state, void *arg);

static int state_accepted(struct state *state, void *arg);

struct regex *regex_compile(struct arena *arena, char *pattern) {
	struct dfa *ret;
	struct arena *ta;
	struct nfa_node *nfa;
	struct state *initial_state;

	ta = arena_new();
	nfa = nfa_compile(ta, pattern);
	initial_state = state_new(arena);
	state_append(initial_state, nfa);

	ret = dfa_new(arena, NFA_MAX_TRANSITIONS, initial_state,
			enclose,
			transition,
			followups,
			state_accepted,
			NULL);

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
	struct hashset *added;
	struct state *state;
	struct hashset *new;
};

static void enclose(struct state *state, void *arg) {
	struct hashset *added, *unchecked, *new;
	struct state_item *iter;
	struct arena *pm_arena, *tmp_arena, *tmp2_arena;
	struct enclose_closure closure;

	(void) arg;

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
		struct state *state, int c, void *arg) {
	struct state *ret;
	struct state_item *state_iter;
	struct nfa_list *nfa_iter;
	struct nfa_node *node;

	(void) arg;

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
	struct nfa_node *node;

	(void) arg;

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
