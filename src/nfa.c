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

/* lowest level compile function, compiles a group */
static long compile_group(struct nfa *nfa, char *string, long len,
		long *rs, long *re);

/* lowest level compile function, compiles a character set (i.e. [a-zA-Z0-9] */
static long compile_cset(struct nfa *nfa, char *string, long len,
		long *rs, long *re);

/* compile_cset helper function, compiles a part of a cset */
static long compile_cset_section(struct nfa *nfa, long start, long end,
		char *string, long len);

/* compile_cset helper function, compiles a cset range. for example, for the
 * range [a-z], we call compile_cset_range(nfa, start, end, 'a', 'z')
 *
 * returns 0 on success, and non-zero on error*/
static int compile_cset_range(struct nfa *nfa, long start, long end,
		char low, char high);

/* compile_cset_range helper function, compiles a cset range given some
 * universe. for example, with the universe "abcdefghijklmnopqrstuvwxyz", we can
 * recognize lowercase letters only. */
static int compile_cset_range6(struct nfa *nfa, long start, long end,
		char low, char high, char *universe);

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
	switch (string[0]) {
	case '(':
		return compile_group(nfa, string, len, rs, re);
	case '[':
		return compile_cset(nfa, string, len, rs, re);
	case ')': case ']':
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
}

static long compile_group(struct nfa *nfa, char *string, long len,
		long *rs, long *re) {
	int l;
	long i;

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

/* mathematically this function is unnecessary as character sets like [0-9] can
 * be matched with choices like (0|1|2|...|9), but in practice these large
 * choices create tons of orphan nodes in the dfa which waste a lot of memory
 * and spend a lot of cpu resources during compilation. also, supporting
 * character sets just leads to shorter regexes. */
static long compile_cset(struct nfa *nfa, char *string, long len,
		long *rs, long *re) {
	long i, d;

	*rs = new_node(nfa);
	*re = new_node(nfa);

	for (i = 1; i < len; ++i) {
		if (string[i] == ']') {
			return i+1;
		}
		d = compile_cset_section(nfa, *rs, *re, string+i, len-i);
		if (d < 0) {
			return -1;
		}
		i += d-1;
	}
	return -1;
}

static long compile_cset_section(struct nfa *nfa, long start, long end,
		char *string, long len) {
	int ch;
	long r;
	if (string[0] == '\\') {
		if (len == 1) {
			return -1;
		}
		ch = (int) (unsigned char) string[1];
		r = 2;
		goto know_char;
	}
	if (len >= 3 && string[1] == '-') {
		return compile_cset_range(nfa, start, end, string[0], string[2])
			? -1:3;
	}
	ch = (int) (unsigned char) string[0];
	r = 1;
know_char:
	add_path(nfa, start, end, ch);
	return r;
}

static int compile_cset_range(struct nfa *nfa, long start, long end,
		char low, char high) {
#define cset_universe(universe) \
	if (!compile_cset_range6(nfa, start, end, low, high, universe)) { \
		return 0; \
	}
	cset_universe("abcdefghijklmnopqrstuvwxyz");
	cset_universe("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	cset_universe("0123456789");
#undef cset_universe
	return 1;
}

static int compile_cset_range6(struct nfa *nfa, long start, long end,
		char low, char high, char *universe) {
	char *i1, *i2;
	i1 = strchr(universe, low);
	if (i1 == NULL) {
		return 1;
	}
	i2 = strchr(universe, high);
	if (i2 == NULL) {
		return 1;
	}

	while (i1 <= i2) {
		add_path(nfa, start, end, (int) (unsigned char) *i1);
		++i1;
	}
	return 0;
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
