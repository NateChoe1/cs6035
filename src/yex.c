#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "dfa.h"
#include "regex.h"
#include "strmap.h"
#include "getopt.h"

/* IMPL-DEF: memset(0) resets integers
 *
 * also, there's some weird coroutine stuff at the bottom of this file
 *
 * https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
 *
 * also, i'm assuming that for all digits, 'x' - '0' = x, and for all hex digits
 * 'x' - 'a' = x - 10
 * */

struct parse_state {
	/* progress state */
	int parse_char_progress;
	int parse_definitions_progress;

	/* parse_definitions state */
	int in_def_block;

	/* global state */
	long i;
	int ret;
	int eof;
	char line[512];
	size_t line_len;
	struct arena *arena;

	/* info about the grammar */

	enum {
		POINTER,
		ARRAY
	} yytext_type;

	struct strmap *substitutions;
	long output_size;

	/* shared states, defined with %s */
	char **sh_states;
	long sh_states_count;
	long sh_states_alloc;

	/* exclusive states, defined with %x*/
	char **ex_states;
	long ex_states_count;
	long ex_states_alloc;
};

static int write_output(char **inputs, FILE *output, int verbose);
static int parse_file(struct parse_state *state, FILE *input, FILE *output);

#define ERROR "[\x1b[41;30;1mERROR\x1b[0m]\t"
#define INFO "[\x1b[33;1mINFO\x1b[0m]\t"
static void report(char *level, char *message, ...);

#define RESET_STATE -2
static int parse_char(struct parse_state *state, int c, FILE *output);
static int parse_definitions(struct parse_state *state, int c, FILE *output);
static int parse_definition_line(struct parse_state *state, FILE *output);
static int parse_substitution(struct parse_state *state);
static char *read_ere(struct arena *arena, char *ere, char **endptr);
static int read_escape(char *s, char *ret);
static int read_octal(char *s, char *ret);
static int read_hex(char *s, char *ret);
static int read_states(struct arena *arena,
		char *line, char ***states, long *len, long *alloc);

int yex_main(int argc, char **argv) {
	FILE *output;
	int c, verbose, to_stdout;

	verbose = to_stdout = 0;
	for (;;) {
		c = getopt(argc, argv, "tnv");
		switch (c) {
		case 't':
			to_stdout = 1;
			break;
		case 'n':
			verbose = 0;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
			fprintf(stderr, "Unknown option -%c\n", optopt);
			goto bad_arg;
		case -1:
			goto got_args;
		}
	}
got_args:

	if (optind >= argc) {
		fputs("Missing input file\n", stderr);
		goto bad_arg;
	}

	if (to_stdout) {
		output = stdout;
	} else {
		output = fopen("lex.yy.c", "w");
	}
	if (output == NULL) {
		fputs("Failed to open output file\n", stderr);
		return 1;
	}

	c = write_output(argv + optind, output, verbose);
	fclose(output);
	return c;
bad_arg:
	fprintf(stderr, "Usage: %s [-t] [-n|-v] [file...]\n",
			argv[0]);
	return 1;

	/* TODO: delete me */
	read_ere(NULL, NULL, NULL);
}

static int write_output(char **inputs, FILE *output, int verbose) {
	int i, ret;
	struct parse_state state;
	FILE *input;

	/* the first input is to initialize the state */
	memset(&state, 0, sizeof(state));
	parse_char(&state, RESET_STATE, output);

	for (i = 0; inputs[i] != NULL; ++i) {
		input = fopen(inputs[i], "r");
		if (input == NULL) {
			fprintf(stderr, "Failed to open input file %s\n",
					inputs[i]);
			return 1;
		}

		ret = parse_file(&state, input, output);
		fclose(input);
		if (ret != 0) {
			return 1;
		}
	}

	if (parse_char(&state, EOF, output) != 0) {
		return 1;
	}

	/* TODO: print summary if verbose */
	(void) verbose;

	return 0;
}

static int parse_file(struct parse_state *state, FILE *input, FILE *output) {
	int c;

	for (;;) {
		c = fgetc(input);
		if (c == EOF) {
			return 0;
		}

		if (parse_char(state, c, output) != 0) {
			return 1;
		}
	}
}

static void report(char *level, char *message, ...) {
	va_list ap;

	va_start(ap, message);
	fputs(level, stderr);
	vfprintf(stderr, message, ap);
}

#define COROUTINE_START(progress) \
	int *p = &progress; \
	if (c == RESET_STATE) { \
		*p = 0; \
	} \
	switch(*p) { \
	case -1: \
		return 1; \
	case 0: \
	((void) 0)
#define COROUTINE_END } do { return 1; } while (0)
#define GET_CHAR do { \
	*p = __LINE__; \
	return 0; \
	case __LINE__: \
	(void) 0; \
} while (0)
#define RETURN(value) do { *p = -1; return value; } while (0)

#define GET_LINE do { \
	state->line_len = 0; \
	for (;;) { \
		if (state->line_len > sizeof(state->line)) { \
			RETURN(1); \
		} \
		GET_CHAR; \
		if (c == '\n') { \
			state->eof = 0; \
			state->line[state->line_len] = '\0'; \
			break; \
		} \
		if (c == EOF) { \
			state->eof = 1; \
			state->line[state->line_len] = '\0'; \
			break; \
		} \
		state->line[state->line_len++] = c; \
	} \
} while (0)

static int parse_char(struct parse_state *state, int c, FILE *output) {
	COROUTINE_START(state->parse_char_progress);

	state->arena = arena_new();

	/* parse definitions section
	 * the first call is there to initialize the definitions part of the
	 * state */
	parse_definitions(state, RESET_STATE, output);
	for (;;) {
		GET_CHAR;
		state->ret = parse_definitions(state, c, output);

		switch (state->ret) {
		case 0:
			continue;
		case -1:
			goto parsed_definitions;
		case 1:
			state->ret = 1;
			goto ret;
		}
	}
parsed_definitions:

	puts("Shared states:");
	for (state->i = 0; state->i < state->sh_states_count; ++state->i) {
		puts(state->sh_states[state->i]);
	}

	puts("Exclusive states:");
	for (state->i = 0; state->i < state->ex_states_count; ++state->i) {
		puts(state->ex_states[state->i]);
	}
	printf("%ld\n", state->output_size);

	state->ret = 0;
ret:
	arena_free(state->arena);
	RETURN(state->ret);

	COROUTINE_END;
}

static int parse_definitions(struct parse_state *state, int c, FILE *output) {
	COROUTINE_START(state->parse_definitions_progress);

	state->in_def_block = 0;
	state->yytext_type = POINTER;

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
		GET_LINE;
		switch (parse_definition_line(state, output)) {
		case -1:
			break;
		case 0:
			RETURN(-1);
		case 1:
			RETURN(1);
		}
		if (state->eof) {
			RETURN(1);
		}
	}

	COROUTINE_END;
}

static int parse_definition_line(struct parse_state *state, FILE *output) {
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

static int parse_substitution(struct parse_state *state) {
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

	/* find end of substitution */
	for (j = 0; state->line[j] != '\0'; ++j) ;
	for (--j; state->line[j] == ' '; --j) ;
	++j;

	/* copy substitution */
	subst = arena_malloc(state->arena, j-i+1);
	memcpy(subst, state->line+i, j-i);
	subst[j-i] = '\0';

	/* add substitution */
	strmap_put(state->substitutions, name, subst);

	return 0;
}

static char *read_ere(struct arena *arena, char *ere, char **endptr) {
	char *ret;
	long i, d, len, alloc;
	int in_q;

	alloc = 32;
	ret = arena_malloc(arena, alloc);
	len = 0;
	in_q = 0;

	for (i = 0;; ++i) {
		if (len >= alloc) {
			alloc *= 2;
			ret = arena_realloc(arena, alloc);
		}

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

		if (ere[i] != '\\') {
			ret[len] = ere[i];
			continue;
		}

		d = read_escape(ere+i, &ret[len]);
		if (d == 0) {
			return NULL;
		}
		++len;
		i += d-1;
	}

	if (in_q) {
		return NULL;
	}

	if (endptr != NULL) {
		*endptr = ere + len;
	}
	return ret;
}

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

static int read_states(struct arena *arena,
		char *line, char ***states, long *len, long *alloc) {
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

#undef COROUTINE_START
#undef COROUTINE_END
#undef GET_CHAR
#undef GET_LINE
