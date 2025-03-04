#include <string.h>

#include "nfa.h"
#include "arena.h"

/* high level compile function. this function literally just detects if a regex
 * contains a choice and defers to compile_local */
static int compile_global(struct arena *arena, char *string, long len,
		struct nfa_node **rs, struct nfa_node **re);

/* low level compile function, this detects sequences and kleene stars */
static int compile_local(struct arena *arena, char *string, long len,
		struct nfa_node **rs, struct nfa_node **re);

/* lower level compile function, this compiles the first character/group of a
 * regex
 *
 * returns the length of the first character/group, or -1 on error */
static long compile_first(struct arena *arena, char *string, long len,
		struct nfa_node **rs, struct nfa_node **re);

/* lowest level compile function, compiles a single character */
static void compile_char(struct arena *arena, char c,
		struct nfa_node **s, struct nfa_node **e);

static struct nfa_node *new_node(struct arena *arena);

static void add_path(struct arena *arena,
		struct nfa_node *source, struct nfa_node *dst, int idx);

struct nfa_node *nfa_compile(struct arena *arena, char *string) {
	struct nfa_node *start, *end;

	if (compile_global(arena, string, strlen(string), &start, &end)) {
		return NULL;
	}
	end->should_accept = 1;
	return start;
}

static int compile_global(struct arena *arena, char *string, long len,
		struct nfa_node **rs, struct nfa_node **re) {
	int l;
	long i;
	struct nfa_node *s1, *e1, *s2, *e2, *sg, *eg;
	i = l = 0;
	for (i = 0; i < len; ++i) {
		if (l < 0) {
			return 1;
		}
		switch (string[i]) {
		case '\\':
			++i;
			continue;
		case '(':
			++l;
			continue;
		case ')':
			--l;
			continue;
		case '|':
			break;
		default:
			continue;
		}

		if (l != 0) {
			continue;
		}

		/* this regex contians a choice */
		sg = new_node(arena);
		eg = new_node(arena);

		/* compile the two options */
		if (compile_local(arena, string, i, &s1, &e1)) {
			return 1;
		}
		if (compile_global(arena, string+i+1, len-i-1, &s2, &e2)) {
			return 1;
		}
		add_path(arena, sg, s1, NFA_EMPTY_IDX);
		add_path(arena, sg, s2, NFA_EMPTY_IDX);
		add_path(arena, e1, eg, NFA_EMPTY_IDX);
		add_path(arena, e2, eg, NFA_EMPTY_IDX);
		*rs = sg;
		*re = eg;
		return 0;
	}

	if (l != 0) {
		return 1;
	}

	/* this regex doesn't have a choice */
	return compile_local(arena, string, len, rs, re);
}

static int compile_local(struct arena *arena, char *string, long len,
		struct nfa_node **rs, struct nfa_node **re) {
	long bound;
	struct nfa_node *fs, *fe, *ts, *te;
	if (len == 0) {
		*rs = *re = new_node(arena);
		return 0;
	}
	bound = compile_first(arena, string, len, &fs, &fe);
	if (bound < 0) {
		return 1;
	}

	if (bound < len && string[bound] == '*') {
		++bound;
		ts = new_node(arena);
		te = new_node(arena);
		add_path(arena, ts, fs, NFA_EMPTY_IDX);
		add_path(arena, fe, te, NFA_EMPTY_IDX);
		add_path(arena, fe, fs, NFA_EMPTY_IDX);
		add_path(arena, ts, te, NFA_EMPTY_IDX);
		fs = ts;
		fe = te;
	}

	if (compile_local(arena, string+bound, len-bound, &ts, &te)) {
		return 1;
	}
	add_path(arena, fe, ts, NFA_EMPTY_IDX);
	*rs = fs;
	*re = te;
	return 0;
}

static long compile_first(struct arena *arena, char *string, long len,
		struct nfa_node **rs, struct nfa_node **re) {
	int l;
	long i;
	switch (string[0]) {
	case '(':
		break;
	case ')':
		return -1;
	case '\\':
		if (len < 2) {
			return -1;
		}
		compile_char(arena, string[1], rs, re);
		return 2;
	default:
		compile_char(arena, string[0], rs, re);
		return 1;
	}

	l = 0;
	for (i = 0; i < len; ++i) {
		if (string[i] == '\\') {
			++i;
			continue;
		}
		if (string[i] == '(') {
			++l;
		}
		if (string[i] == ')') {
			--l;
		}
		if (l < 0) {
			return -1;
		}
		if (l == 0) {
			++i;
			goto found_match;
		}
	}
	return -1;
found_match:
	if (compile_global(arena, string+1, i-2, rs, re)) {
		return -1;
	}
	return i;
}

static void compile_char(struct arena *arena, char c,
		struct nfa_node **s, struct nfa_node **e) {
	*s = new_node(arena);
	*e = new_node(arena);
	add_path(arena, *s, *e, (int) (unsigned char) c);
}

static struct nfa_node *new_node(struct arena *arena) {
	struct nfa_node *ret;
	int i;
	ret = arena_malloc(arena, sizeof(*ret));
	ret->should_accept = 0;
	for (i = 0; i < NFA_MAX_TRANSITIONS; ++i) {
		ret->transitions[i] = NULL;
	}
	return ret;
}

static void add_path(struct arena *arena,
		struct nfa_node *source, struct nfa_node *dst, int idx) {
	struct nfa_list *head;
	head = arena_malloc(arena, sizeof(*head));
	head->node = dst;
	head->next = source->transitions[idx];
	source->transitions[idx] = head;
}
