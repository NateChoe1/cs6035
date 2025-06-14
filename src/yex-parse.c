#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "sb.h"
#include "utils.h"
#include "arena.h"
#include "strmap.h"
#include "yex-parse.h"
#include "coroutine.h"

/* IMPL-DEF: i'm assuming that for all digits, 'x' - '0' = x, and for all hex
 * digits 'x' - 'a' = x - 10
 * */

/* definition section parse functions */
static int parse_definitions(struct yex_parse_state *state,
		int c, FILE *output);
static int parse_definition_line(struct yex_parse_state *state, FILE *output);
static int parse_substitution(struct yex_parse_state *state);
static int read_states(struct arena *arena,
		char *line, char ***states, size_t *len, size_t *alloc);

/* rules section parse functions */
static int parse_rules(struct yex_parse_state *state, int c);
static int parse_rules_closed(struct yex_parse_state *state, int c);
static struct yex_parse_rule *read_ere(struct arena *arena,
		char *ere, struct strmap *substs);
static int read_ere_help(struct yex_parse_rule *rule, struct sb *sb,
		char *ere, struct strmap *substs);
static long read_subst(struct sb *sb, char *ere, struct strmap *substs);
static int read_escape(char *s, char *ret);
static int read_octal(char *s, char *ret);
static int read_hex(char *s, char *ret);

/* yex_parse_char is defined here */
#include "yex-parse.skl.comp"

/* start of definition section parse definitions */

static int parse_definitions(struct yex_parse_state *state,
		int c, FILE *output) {
	COROUTINE_START(state->parse_definitions_progress);

	state->in_def_block = 0;
	state->yytext_type = ARRAY;

	state->substitutions = strmap_new(state->arena);
	state->output_size = 3000;

	state->sh_states_count = 0;
	state->sh_states_alloc = 32;
	state->sh_states = arena_malloc(state->arena,
			state->sh_states_alloc * sizeof(*state->sh_states));

	state->ex_states_count = 0;
	state->ex_states_alloc = 32;
	state->ex_states = arena_malloc(state->arena,
			state->ex_states_alloc * sizeof(*state->ex_states));

	for (;;) {
		COROUTINE_GETL;
		switch (parse_definition_line(state, output)) {
		case -1:
			break;
		case 0:
			COROUTINE_RET(-1);
		case 1:
			COROUTINE_RET(1);
		}
		if (state->eof) {
			COROUTINE_RET(1);
		}
	}

	COROUTINE_END;
}

static int parse_definition_line(struct yex_parse_state *state, FILE *output) {
	if (state->in_def_block) {
		if (strcmp(state->line, "%}")) {
			state->in_def_block = 0;
			return -1;
		}
		goto echo;
	}

	if (strcmp(state->line, "%{") == 0) {
		state->in_def_block = 1;
		return -1;
	}

	if (state->line[0] == ' ') {
		goto echo;
	}

	if (state->line[0] == '\0') {
		return -1;
	}

	if (state->line[0] != '%') {
		return parse_substitution(state);
	}

	if (strcmp(state->line, "%%") == 0) {
		return 0;
	}

	if (strcmp(state->line, "%array") == 0) {
		state->yytext_type = ARRAY;
		return -1;
	}

	if (strcmp(state->line, "%pointer") == 0) {
		state->yytext_type = POINTER;
		return -1;
	}

	switch (state->line[1]) {
	case 'p': case 'n': case 'a': case 'e': case 'k':
		return -1;
	case 's': case 'S':
		return read_states(state->arena, state->line,
				&state->sh_states,
				&state->sh_states_count,
				&state->sh_states_alloc);
	case 'x': case 'X':
		return read_states(state->arena, state->line,
				&state->ex_states,
				&state->ex_states_count,
				&state->ex_states_alloc);
	case 'o':
		state->output_size = atol(state->line+2);
		return -1;
	default:
		return 1;
	}

	return 1;
echo:
	fprintf(output, "%s\n", state->line);
	return -1;
}

static int parse_substitution(struct yex_parse_state *state) {
	long i, j;
	char *name, *subst;

	/* get name boundary */
	if (!isalpha(state->line[0]) && state->line[0] != '_') {
		return 1;
	}
	for (i = 1; isalnum(state->line[i]) || state->line[0] == '_'; ++i) ;

	name = arena_malloc(state->arena, i+1);
	memcpy(name, state->line, i);
	name[i] = '\0';

	/* check if we already have a substitution of the same name */
	if (strmap_get(state->substitutions, name) != NULL) {
		report(ERROR, "Duplicate substitution");
		return 1;
	}

	/* find beginning of substitution */
	for (; state->line[i] == ' '; ++i) ;

	/* find end of substitution
	 *
	 * note that the substitution always continues to the end of the line,
	 * so something like
	 *
	 *    digit_with_space [0-9] a
	 *
	 * would be equivalent to
	 *
	 *    digit_with_space "[0-9] a"
	 *
	 * even though there's a space in the regex. i think this is acceptable
	 * behavior.
	 * */
	for (j = i; state->line[j] != '\0'; ++j) ;

	/* copy substitution */
	subst = arena_malloc(state->arena, j-i+1);
	memcpy(subst, state->line+i, j-i);
	subst[j-i] = '\0';

	/* add substitution */
	strmap_put(state->substitutions, name, subst);

	return -1;
}

static int read_states(struct arena *arena,
		char *line, char ***states, size_t *len, size_t *alloc) {
	long i;
	char *name;

	while (*line != ' ' && *line != '\0') {
		++line;
	}
	while (*line == ' ') {
		++line;
	}

	while (*line) {
		for (i = 0; line[i] != ' ' && line[i] != '\0'; ++i) ;

		name = arena_malloc(arena, i+1);
		memcpy(name, line, i);
		name[i] = '\0';

		if (*len >= *alloc) {
			*alloc *= 2;
			*states = arena_realloc(*states,
					*alloc * sizeof(**states));
		}
		(*states)[(*len)++] = name;

		line += i;

		while (*line == ' ') {
			++line;
		}
	}

	return -1;
}

/* end of definitions section parse definitions */

/* start of rules section parse definitions */

static int parse_rules(struct yex_parse_state *state, int c) {
	COROUTINE_START(state->parse_rules_progress);

	state->rules_count = 0;
	state->rules_alloc = 32;
	state->rules = arena_malloc(state->arena,
			state->rules_alloc * sizeof(*state->rules));

	for (state->pr_l = 0; state->pr_l < 4; ++state->pr_l) {
		COROUTINE_GETC;
		if (c == COROUTINE_EOF) {
			goto end;
		}
		state->pr_c[state->pr_l] = c;
	}

	/* pr_c contains the last 4 characters read
	 * if pr_c ever equals "\n%%\n", we know we've hit the end of the rules
	 * section and can move on.
	 *
	 * note that there are some problems with files like this:
	 *
	 * re {
	 *   puts("hi");
	 * %%
	 * }
	 *
	 * in this case, the rules section ends inside of a code block, which
	 * isn't ideal. posix seems to be ambiguous on this.
	 *
	 * pr_l contains the number of characters read but not processed. we
	 * need to keep track of this because the according to posix, "the
	 * second '%%' [which ends the rules section] is required _only if user
	 * subroutines follow_". this means that if we hit eof without hitting
	 * "\n%%\n", we have to flush the rest of the output.
	 *
	 * this could technically be made slightly more efficient with a
	 * circular memory buffer, but for 4 characters that seems like
	 * overkill.
	 * */

	parse_rules_closed(state, COROUTINE_RESET);
	for (;;) {
		if (memcmp(state->pr_c, "\n%%\n", 4) == 0) {
			state->pr_l = 0;
			break;
		}

		parse_rules_closed(state, state->pr_c[0]);
		memmove(state->pr_c, state->pr_c+1, 3);
		COROUTINE_GETC;
		if (c == COROUTINE_EOF) {
			state->pr_l = 3;
			break;
		}
		state->pr_c[3] = c;
	}
end:
	for (state->pr_i = 0; state->pr_i < state->pr_l; ++state->pr_i) {
		parse_rules_closed(state, state->pr_c[state->pr_i]);
	}

	parse_rules_closed(state, '\n');
	COROUTINE_RET(parse_rules_closed(state, COROUTINE_EOF));

	COROUTINE_END;
}

/* this function receives eof at the end of the rules section, regardless of
 * whether or not user subroutines follow */
static int parse_rules_closed(struct yex_parse_state *state, int c) {
	COROUTINE_START(state->parse_rules_closed_progress);

	for (;;) {
		COROUTINE_GETL;

		if (state->line_len == 0) {
			goto skip_parse;
		}

		if (state->rules_count >= state->rules_alloc) {
			state->rules_alloc *= 2;
			state->rules = arena_realloc(state->rules,
					state->rules_alloc *
					sizeof(*state->rules));
		}

		state->rules[state->rules_count] = read_ere(state->arena,
				state->line, state->substitutions);
		if (state->rules[state->rules_count] == NULL) {
			fputs("Failed to read regex\n", stderr);
			COROUTINE_RET(1);
		}

		printf("%d %s\t%s\n", state->rules[state->rules_count]->anchored,
				state->rules[state->rules_count]->re,
				state->rules[state->rules_count]->trail);

		++state->rules_count;

skip_parse:
		if (state->eof) {
			break;
		}
	}

	COROUTINE_RET(-1);
	COROUTINE_END;
}

static struct yex_parse_rule *read_ere(struct arena *arena,
		char *ere, struct strmap *substs) {
	struct sb *sb;
	struct yex_parse_rule *ret;

	sb = sb_new(arena);
	ret = arena_malloc(arena, sizeof(*ret));

	if (read_ere_help(ret, sb, ere, substs)) {
		return NULL;
	}

	return ret;
}

/* appends an ere to `sb` and writes to `rule`
 *
 * if rule is NULL, then we assume that we're reading a substitution and throw
 * an error on anchors, trailing context, and extra characters after our regex
 *
 * returns 1 on error, 0 on success
 * */
static int read_ere_help(struct yex_parse_rule *rule, struct sb *sb,
		char *ere, struct strmap *substs) {
	long i, d;
	int in_q;
	char c;

	in_q = 0;

	if (ere[0] == '^') {
		if (rule == NULL) {
			return 1;
		}
		rule->anchored = 1;
		++ere;
	} else if (rule != NULL) {
		rule->anchored = 0;
	}

	for (i = 0;; ++i) {
		if (ere[i] == '\0') {
			break;
		}
		if (ere[i] == ' ' && !in_q) {
			break;
		}

		if (ere[i] == '"') {
			in_q = !in_q;
			continue;
		}

		if (ere[i] == '{') {
			d = read_subst(sb, ere+i, substs);
			switch (d) {
			case 0:
				goto not_subst;
			case -1:
				return 1;
			}
			i += d-1;
			continue;
		}
not_subst:

		switch (ere[i]) {
		case '$':
			if (rule == NULL) {
				return 1;
			}
			rule->trail = "\n";
			continue;
		case '/':
			if (rule == NULL) {
				return 1;
			}
			sb_append(sb, '\0');
			rule->trail = sb_read(sb) + sb->len;
			continue;
		case '\\':
			break;
		default:
			sb_append(sb, ere[i]);
			continue;
		}

		d = read_escape(ere+i, &c);
		if (d == 0) {
			return 1;
		}
		sb_append(sb, c);
		i += d-1;
	}

	if (in_q) {
		return 1;
	}

	if (rule == NULL && ere[i] != '\0') {
		return 1;
	}
	if (rule != NULL) {
		rule->re = sb_read(sb);
		rule->action = ere + i;
	}

	return 0;
}

/* parses the first substitution in ere, returns 0 on error
 *
 * for example, if ere == "{digit}", and "digit" was a substitution defined
 * earlier, this function will append ([0-9]) to sb, or whatever the actual
 * regex was */
static long read_subst(struct sb *sb, char *ere, struct strmap *substs) {
	long d;
	char *k, *v;

	for (d = 0; ere[d] != '\0' && ere[d] != '}'; ++d) ;

	if (ere[d] == '\0') {
		return 0;
	}

	++d;
	k = xmalloc(d-1);
	memcpy(k, ere+1, d-2);
	k[d-2] = '\0';

	v = strmap_get(substs, k);
	if (v == NULL) {
		d = 0;
		goto end;
	}

	sb_append(sb, '(');
	if (read_ere_help(NULL, sb, v, substs)) {
		fprintf(stderr, "Error while expanding substition `%s`\n", k);
		d = -1;
		goto end;
	}
	sb_append(sb, ')');

end:
	free(k);
	return d;
}

/* returns the length of the escaped character at the beginning of s, as well as
 * its literal value in *ret
 *
 * for example, if s == "\\n", this function sets *ret to '\n' and returns 2
 *
 * returns 0 on error */
static int read_escape(char *s, char *ret) {
	int d;

	if (isdigit(s[1])) {
		return read_octal(s+1, ret)+1;
	}

	switch (s[1]) {
	case '\0':
		return 0;
	case '\\':
		*ret = '\\';
		return 2;
	case 'a':
		*ret = '\a';
		return 2;
	case 'b':
		*ret = '\b';
		return 2;
	case 'f':
		*ret = '\f';
		return 2;
	case 'n':
		*ret = '\n';
		return 2;
	case 'r':
		*ret = '\r';
		return 2;
	case 't':
		*ret = '\t';
		return 2;
	case 'v':
		*ret = '\v';
		return 2;
	case 'x':
		d = read_hex(s+1, ret);
		if (d == 0) {
			return 0;
		}
		return d+1;
	}

	*ret = s[1];
	return 2;
}

static int read_octal(char *s, char *ret) {
	int i, v;

	v = 0;
	for (i = 0; i < 3; ++i) {
		if (s[i] < '0' || '7' < s[i]) {
			break;
		}
		v *= 8;
		v += s[i] - '0';
	}

	*ret = (char) v;

	return i;
}

static int read_hex(char *s, char *ret) {
	int i, v;

	v = 0;
	for (i = 0;; ++i) {
		if (!isxdigit(s[i])) {
			break;
		}
		v *= 16;
		if (isdigit(s[i])) {
			v += s[i] - '0';
		} else if (isupper(s[i])) {
			v += s[i] - 'A' + 10;
		} else {
			v += s[i] - 'a' + 10;
		}
	}

	*ret = (char) v;

	return i;
}

/* end of rules section parse definitions */

/* shutting up gcc warnings */
static void shutup2(void);
static void shutup1(void) {
	shutup2();
	read_ere(NULL, NULL, NULL);
	parse_rules(NULL, 0);
	parse_substitution(NULL);
}
static void shutup2(void) {
	shutup1();
}
