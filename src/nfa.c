#include <string.h>

#include "nfa.h"
#include "arena.h"

/* high level compile function. this function literally just detects if a regex
 * contains a choice and defers to compile_local */
static int compile_global(struct nfa *nfa, char *string, long len,
		long *rs, long *re);

/* low level compile function, this detects sequences and kleene stars */
static int compile_local(struct nfa *nfa, char *string, long len,
		long *rs, long *re);

/* lower level compile function, this compiles the first character/group of a
 * regex
 *
 * returns the length of the first character/group, or -1 on error */
static long compile_first(struct nfa *nfa, char *string, long len,
		long *rs, long *re);

/* lowest level compile function, compiles a single character */
static void compile_char(struct nfa *nfa, char c, long *s, long *e);

static long new_node(struct nfa *nfa);

static void add_path(struct nfa *nfa, long source, long dst, int idx);

struct nfa *nfa_compile(struct arena *arena, char *string) {
	struct nfa *ret;
	long start, end;

	ret = arena_malloc(arena, sizeof(*ret));
	ret->arena = arena;
	ret->num_nodes = 0;
	ret->alloc = 32;
	ret->nodes = arena_malloc(arena, ret->alloc * sizeof(*ret->nodes));

	if (compile_global(ret, string, strlen(string), &start, &end)) {
		return NULL;
	}

	ret->start_node = start;
	ret->nodes[end].should_accept = 1;
	return ret;
}

static int compile_global(struct nfa *nfa, char *string, long len,
		long *rs, long *re) {
	int l;
	long i, s1, e1, s2, e2, sg, eg;
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

		/* this regex contains a choice */
		sg = new_node(nfa);
		eg = new_node(nfa);

		/* compile the two options */
		if (compile_local(nfa, string, i, &s1, &e1)) {
			return 1;
		}
		if (compile_global(nfa, string+i+1, len-i-1, &s2, &e2)) {
			return 1;
		}
		add_path(nfa, sg, s1, NFA_EMPTY_IDX);
		add_path(nfa, sg, s2, NFA_EMPTY_IDX);
		add_path(nfa, e1, eg, NFA_EMPTY_IDX);
		add_path(nfa, e2, eg, NFA_EMPTY_IDX);
		*rs = sg;
		*re = eg;
		return 0;
	}

	if (l != 0) {
		return 1;
	}

	/* this regex doesn't have a choice */
	return compile_local(nfa, string, len, rs, re);
}

static int compile_local(struct nfa *nfa, char *string, long len,
		long *rs, long *re) {
	long bound, fs, fe, ts, te;
	if (len == 0) {
		*rs = *re = new_node(nfa);
		return 0;
	}
	bound = compile_first(nfa, string, len, &fs, &fe);
	if (bound < 0) {
		return 1;
	}

	if (bound < len && string[bound] == '*') {
		++bound;
		ts = new_node(nfa);
		te = new_node(nfa);
		add_path(nfa, ts, fs, NFA_EMPTY_IDX);
		add_path(nfa, fe, te, NFA_EMPTY_IDX);
		add_path(nfa, fe, fs, NFA_EMPTY_IDX);
		add_path(nfa, ts, te, NFA_EMPTY_IDX);
		fs = ts;
		fe = te;
	}

	if (compile_local(nfa, string+bound, len-bound, &ts, &te)) {
		return 1;
	}
	add_path(nfa, fe, ts, NFA_EMPTY_IDX);
	*rs = fs;
	*re = te;
	return 0;
}

static long compile_first(struct nfa *nfa, char *string, long len,
		long *rs, long *re) {
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
		compile_char(nfa, string[1], rs, re);
		return 2;
	default:
		compile_char(nfa, string[0], rs, re);
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
	if (compile_global(nfa, string+1, i-2, rs, re)) {
		return -1;
	}
	return i;
}

static void compile_char(struct nfa *nfa, char c, long *s, long *e) {
	*s = new_node(nfa);
	*e = new_node(nfa);
	add_path(nfa, *s, *e, (int) (unsigned char) c);
}

static long new_node(struct nfa *nfa) {
	int i;

	if (nfa->num_nodes >= nfa->alloc) {
		nfa->alloc *= 2;
		nfa->nodes = arena_realloc(nfa->nodes,
				nfa->alloc * sizeof(*nfa->nodes));
	}

	nfa->nodes[nfa->num_nodes].should_accept = 0;
	for (i = 0; i < NFA_MAX_TRANSITIONS; ++i) {
		nfa->nodes[nfa->num_nodes].transitions[i] = NULL;
	}

	return nfa->num_nodes++;
}

static void add_path(struct nfa *nfa, long source, long dst, int idx) {
	struct nfa_list *head;
	head = arena_malloc(nfa->arena, sizeof(*head));
	head->node = dst;
	head->next = nfa->nodes[source].transitions[idx];
	nfa->nodes[source].transitions[idx] = head;
}
