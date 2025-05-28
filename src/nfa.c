#include <ctype.h>
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

/* parse_cset helper function, parses a part of a cset */
static long parse_cset_section(char *matches, char *string, long len);

/* compile_cset helper function, compiles a character class (i.e. [:alnum:]) */
static long compile_cclass(struct nfa *nfa, long start, long end,
		char *string, long len);

/* compile_cclass helper function, compiles an expanded character class (i.e.
 * [:xdigit:] would expand to "0123456789abcdefABCDEF" */
static void compile_cclass_expanded(struct nfa *nfa, long start, long end,
		char *members);

/* compile_cclass helper function, checks if a string starts with a character
 * class and returns the length of the match. for example, "[:alnum:]$", "alnum"
 * would return 9, because strlen("[:alnum:]") == 9. if there is no match, this
 * function returns 0 */
static long class_matches(char *s, char *name, long len);

static void compile_dot(struct nfa *nfa, long start, long end);

/* compile_cset helper function, compiles a cset range. for example, for the
 * range [a-z], we call parse_cset_range(nfa, start, end, 'a', 'z')
 *
 * returns 0 on success, and non-zero on error*/
static int parse_cset_range(char *matches, char low, char high);

/* parse_cset_range helper function, parses a cset range given some universe.
 * for example, with the universe "abcdefghijklmnopqrstuvwxyz", we can recognize
 * lowercase letters only. */
static int parse_cset_range4(char *matches,
		char low, char high, char *universe);

/* lowest level compile function, compiles a single character */
static void compile_char(struct nfa *nfa, char c, long *s, long *e);

/* compile_local helper function
 *
 * handles an interval given some known start bound and two nodes with a
 * precompiled instance of the first regex
 *
 * returns the new bound, or -1 on error
 *
 * TODO: this interface feels super hacky
 * */
static long handle_interval(struct nfa *nfa, char *string, long bound, long len,
		long *fs, long *fe);

/* parses an interval expression, i.e. {5,7}
 *
 * low is set to the lower bound of the interval, high is set to the upper
 * bound, or -1 for expressions like {1,}
 *
 * low is set to -1 on error
 *
 * returns the length of the match, or 0 on error
 * */
static long parse_interval(char *string, long len, long *low, long *high);

/* reads a base 10 number from a string, or -1 if there is no number
 *
 * if mlen isn't NULL, the match len is stored in *mlen
 * */
static long read_num(char *string, long len, long *mlen);

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

	if (bound < len && strchr("*+?", string[bound]) != NULL) {
		ts = new_node(nfa);
		te = new_node(nfa);
		add_path(nfa, ts, fs, NFA_EMPTY_IDX);
		add_path(nfa, fe, te, NFA_EMPTY_IDX);
		if (string[bound] != '?') {
			add_path(nfa, fe, fs, NFA_EMPTY_IDX);
		}
		if (string[bound] != '+') {
			add_path(nfa, ts, te, NFA_EMPTY_IDX);
		}

		fs = ts;
		fe = te;
		++bound;
	} else if (bound < len && string[bound] == '{') {
		bound = handle_interval(nfa, string, bound, len, &fs, &fe);
		if (bound == -1) {
			return 1;
		}
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
	case '.':
		*rs = new_node(nfa);
		*re = new_node(nfa);
		compile_dot(nfa, *rs, *re);
		return 1;
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

static long compile_cset(struct nfa *nfa, char *string, long len,
		long *rs, long *re) {
	int invert;
	long ret, i, d, o;
	char matches[NFA_NUM_REAL_CHARS];

	*rs = new_node(nfa);
	*re = new_node(nfa);

	/* handle character classes like [:alnum:] */
	if (len >= 4 && strchr("=:.", string[1]) != NULL) {
		return compile_cclass(nfa, *rs, *re, string, len);
	}

	if (string[1] == '^') {
		o = 2;
		invert = 1;
	} else {
		o = 1;
		invert = 0;
	}

	memset(matches, 0, sizeof(matches));

	for (i = 1; i < len; ++i) {
		if (i > o && string[i] == ']') {
			ret = i+1;
			goto populate;
		}
		d = parse_cset_section(matches, string+i, len-i);
		if (d < 0) {
			return -1;
		}
		i += d-1;
	}
	return -1;

populate:
	for (i = 0; i < NFA_NUM_REAL_CHARS; ++i) {
		if (!invert && matches[i]) {
			add_path(nfa, *rs, *re, i);
		}
		if (!invert) {
			continue;
		}

		if (i == '\n') {
			continue;
		}
		if (!matches[i]) {
			add_path(nfa, *rs, *re, i);
		}
	}

	return ret;
}

static long parse_cset_section(char *matches, char *string, long len) {
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
		return parse_cset_range(matches, string[0], string[2]) ? -1:3;
	}
	ch = (int) (unsigned char) string[0];
	r = 1;
know_char:
	matches[ch] = 1;
	return r;
}

static long compile_cclass(struct nfa *nfa, long start, long end,
		char *string, long len) {
	int mlen;

	if ((mlen = class_matches(string, "alnum", len))) {
		compile_cclass_expanded(nfa, start, end,
				"abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"012345789");
		return mlen;
	}

	if ((mlen = class_matches(string, "alpha", len))) {
		compile_cclass_expanded(nfa, start, end,
				"abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		return mlen;
	}

	if ((mlen = class_matches(string, "blank", len))) {
		compile_cclass_expanded(nfa, start, end, " \t");
		return mlen;
	}

	if ((mlen = class_matches(string, "cntrl", len))) {
		/* according to posix, there are no characters that are required
		 * to be in the "cntrl" character class, only characters that
		 * are forbidden from being in it. to remain as agnostic as
		 * possible, i'm declaring that there are no characters in the
		 * cntrl class.
		 *
		 * this same argument could technically also apply to the
		 * "punct" class as well, but the "cntrl" class feels much less
		 * important to me than the "punct" class from a regular
		 * expressions standpoint.
		 *
		 * https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html
		 * */
		return mlen;
	}

	if ((mlen = class_matches(string, "digit", len))) {
		compile_cclass_expanded(nfa, start, end, "0123456789");
		return mlen;
	}

	if ((mlen = class_matches(string, "graph", len))) {
		compile_cclass_expanded(nfa, start, end,
				"abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"0123456789"
				"|\"#$%&'()*+,-./<=>?@[\\]^_`{|}~");
		return mlen;
	}

	if ((mlen = class_matches(string, "lower", len))) {
		compile_cclass_expanded(nfa, start, end,
				"abcdefghijklmnopqrstuvwxyz");
		return mlen;
	}

	if ((mlen = class_matches(string, "print", len))) {
		compile_cclass_expanded(nfa, start, end,
				"abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"0123456789"
				"|\"#$%&'()*+,-./<=>?@[\\]^_`{|}~"
				" ");
		return mlen;
	}

	if ((mlen = class_matches(string, "punct", len))) {
		compile_cclass_expanded(nfa, start, end,
				"|\"#$%&'()*+,-./<=>?@[\\]^_`{|}~");
		return mlen;
	}

	if ((mlen = class_matches(string, "space", len))) {
		/* not included: <form-feed>, <newline>, <carriage-return>,
		 * <vertical-tab> */
		compile_cclass_expanded(nfa, start, end,
				" \t");
		return mlen;
	}

	if ((mlen = class_matches(string, "upper", len))) {
		compile_cclass_expanded(nfa, start, end,
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		return mlen;
	}

	if ((mlen = class_matches(string, "xdigit", len))) {
		compile_cclass_expanded(nfa, start, end,
				"0123456789abcdefABCDEF");
		return mlen;
	}

	return -1;
}

static void compile_cclass_expanded(struct nfa *nfa, long start, long end,
		char *members) {
	while (*members) {
		add_path(nfa, start, end, *members);
		++members;
	}
}

static long class_matches(char *s, char *name, long len) {
	long i;
	char sep;

	sep = s[1];
	for (i = 0; name[i] && i + 4 < len; ++i) {
		if (name[i] != s[i+2]) {
			return 0;
		}
	}

	if (s[i+2] == sep && s[i+3] == ']') {
		return i+4;
	}
	return 0;
}

static void compile_dot(struct nfa *nfa, long start, long end) {
	int i;

	/*   A <period> ('.'), when used outside a bracket expression, is an ERE
	 *   that shall match any character in the supported character set
	 *   except NUL.
	 *
	 *   https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap09.html#tag_09_04_04
	 *
	 * also note:
	 *
	 *   A literal <newline> cannot occur within an ERE; the escape sequence
	 *   '\n' can be used to represent a <newline>. A <newline> shall not be
	 *   matched by a period operator.
	 *
	 *   https://pubs.opengroup.org/onlinepubs/9799919799/
	 * */
	for (i = 1; i < NFA_NUM_REAL_CHARS; ++i) {
		if (i == '\n') {
			continue;
		}
		add_path(nfa, start, end, i);
	}
}

static int parse_cset_range(char *matches, char low, char high) {
#define cset_universe(universe) \
	if (!parse_cset_range4(matches, low, high, universe)) { \
		return 0; \
	}
	cset_universe("abcdefghijklmnopqrstuvwxyz");
	cset_universe("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	cset_universe("0123456789");
#undef cset_universe
	return 1;
}

static int parse_cset_range4(char *matches,
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
		matches[(int) (unsigned char) *i1] = 1;
		++i1;
	}
	return 0;
}

static void compile_char(struct nfa *nfa, char c, long *s, long *e) {
	*s = new_node(nfa);
	*e = new_node(nfa);
	add_path(nfa, *s, *e, (int) (unsigned char) c);
}

static long handle_interval(struct nfa *nfa, char *string, long bound, long len,
		long *fs, long *fe) {
	int fresh;
	long low, high, ps, pe, n, i;

	bound += parse_interval(string+bound, len-bound, &low, &high);
	if (low == -1) {
		goto error;
	}

	ps = *fs;
	pe = *fe;

	*fs = new_node(nfa);
	*fe = new_node(nfa);
	n = *fs;
	fresh = 1;

	/* a{5,[something]} => aaaaa [something] */
	for (i = 0; i < low; ++i) {
		if (!fresh) {
			compile_first(nfa, string, len, &ps, &pe);
		}

		add_path(nfa, n, ps, NFA_EMPTY_IDX);
		n = pe;

		fresh = 0;
	}

	/* a{5,} => aaaaa+ */
	if (high == -1) {
		if (low == 0) {
			add_path(nfa, *fs, ps, NFA_EMPTY_IDX);
			add_path(nfa, *fs, *fe, NFA_EMPTY_IDX);
		}
		n = new_node(nfa);
		add_path(nfa, pe, n, NFA_EMPTY_IDX);
		add_path(nfa, n, ps, NFA_EMPTY_IDX);
		add_path(nfa, n, *fe, NFA_EMPTY_IDX);
		return bound;
	}

	/* {n,m} cases */
	for (i = 0; i < high-low; ++i) {
		if (!fresh) {
			compile_first(nfa, string, len, &ps, &pe);
		}

		add_path(nfa, n, ps, NFA_EMPTY_IDX);
		add_path(nfa, n, *fe, NFA_EMPTY_IDX);
		n = pe;

		fresh = 0;
	}
	add_path(nfa, pe, *fe, NFA_EMPTY_IDX);

	return bound;
error:
	return -1;
}

static long parse_interval(char *string, long len, long *low, long *high) {
	long i, diff;

	/* read the initial '{' */
	if (len < 1 || string[0] != '{') {
		goto error;
	}
	i = 1;

	/* read the lower bound */
	*low = read_num(string+i, len-i, &diff);
	if (*low == -1) {
		goto error;
	}
	if ((i += diff) >= len) {
		goto error;
	}

	/* handle {n} intevals */
	if (string[i] == '}') {
		*high = *low;
		return i+1;
	}

	if (string[i] != ',') {
		goto error;
	}

	if (++i >= len) {
		goto error;
	}

	/* handle {n,} intervals */
	if (string[i] == '}') {
		*high = -1;
		return i+1;
	}

	/* handle {n,m} intervals */
	*high = read_num(string+i, len-i, &diff);
	if (*high == -1) {
		goto error;
	}
	if ((i += diff) >= len) {
		goto error;
	}
	if (string[i] != '}') {
		goto error;
	}
	return i+1;
error:
	*low = -1;
	return 0;
}

static long read_num(char *string, long len, long *mlen) {
	long i, ret;

	if (len <= 0 || !isdigit(string[0])) {
		return -1;
	}

	ret = 0;
	for (i = 0; i < len && isdigit(string[i]); ++i) {
		ret *= 10;

		/* IMPL-DEF: assuming that digit characters - '0' gives us their
		 * value*/
		ret += string[i] - '0';
	}

	if (mlen != NULL) {
		*mlen = i;
	}

	return ret;
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
