#include <stdio.h>

#include "dfa.h"
#include "regex.h"

struct pattern_list {
	struct dfa *regex;
	char *code;
	int id;
	struct pattern_list *next;
};

static char *read_line(FILE *input, struct arena *arena);
static struct pattern_list *read_patterns(FILE *input, struct arena *arena);
static int write_header(FILE *input, FILE *output);
static void swallow_line(FILE *input);
static void write_autocode(FILE *output, struct pattern_list *patterns);
static void transition_pattern(FILE *output, struct pattern_list *pattern);
static void transition_state(FILE *output, struct dfa *dfa, int state, int id);
static void run_actions(FILE *output, struct pattern_list *pattern);

int yex_main(int argc, char **argv) {
	FILE *input, *output;
	struct arena *ra;
	struct pattern_list *patterns;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s [input.yex] [output.c]\n", argv[0]);
		return 1;
	}

	input = fopen(argv[1], "r");
	output = fopen(argv[2], "w");

	if (write_header(input, output) != 0) {
		return 1;
	}
	swallow_line(input);
	ra = arena_new();
	patterns = read_patterns(input, ra);

	write_autocode(output, patterns);
	return 0;
}

static char *read_line(FILE *input, struct arena *arena) {
	char *ret;
	size_t len, alloc;
	int c;

	c = fgetc(input);
	if (c == EOF) {
		return NULL;
	}
	ungetc(c, input);

	len = 0;
	alloc = 80;
	ret = arena_malloc(arena, alloc);
	for (;;) {
		if (alloc >= len) {
			alloc *= 2;
			ret = arena_realloc(ret, alloc);
		}

		c = fgetc(input);
		if (c != '\n' && c != EOF) {
			ret[len++] = c;
			continue;
		}
		ret[len] = '\0';
		return ret;
	}
}

static struct pattern_list *read_patterns(FILE *input, struct arena *arena) {
	struct arena *tmp;
	char *pattern, *code;
	struct pattern_list *ret, *cur;
	int i;

	tmp = arena_new();
	ret = NULL;
	i = 0;

	for (;;) {
		pattern = read_line(input, tmp);
		code = read_line(input, arena);
		if (pattern == NULL || code == NULL) {
			break;
		}
		cur = arena_malloc(arena, sizeof(*cur));
		cur->regex = (struct dfa *) regex_compile(arena, pattern);
		cur->code = code;
		cur->next = ret;
		cur->id = i++;
		ret = cur;
	}

	arena_free(tmp);

	return ret;
}

static int write_header(FILE *input, FILE *output) {
	int c;

	for (;;) {
		c = fgetc(input);
		if (c == EOF) {
			fputs("Input doesn't contian a header!", stderr);
			return 1;
		}
		if (c != '%') {
			goto norm;
		}
		c = fgetc(input);
		if (c != '%') {
			ungetc(c, input);
			goto norm;
		}
		return 0;
norm:
		fputc(c, output);
	}
}

static void swallow_line(FILE *input) {
	int c;
	for (;;) {
		c = fgetc(input);
		if (c == '\n' || c == EOF) {
			return;
		}
	}
}

/* it is possible to do all of this parsing in O(n) time by unifying all of our
 * dfas into a single dfa, but that would use a ridiculous amount of memory. */
static void write_autocode(FILE *output, struct pattern_list *patterns) {
	struct pattern_list *iter;

	fputs("#include <yex.h>\n", output);
	fputs("int YEX_NAME(struct yex_buffer *buff) {\n", output);

	iter = patterns;
	while (iter != NULL) {
		fprintf(output, "long state_%d = 0;\n", iter->id);
		iter = iter->next;
	}
	fputs("unsigned long last_match = buff->parsed;\n", output);
	fputs("int last_regex = -1;\n", output);
	fputs("unsigned long i;\n", output);
	fputs("int c, match_possible;\n", output);

	fputs("if (buff->parsed == buff->length) { return YEX_EOF; }\n",
			output);

	fputs("for (i = buff->parsed; i < buff->length; ++i) {\n", output);
	fputs(  "match_possible = 0;\n", output);
	fputs(  "c = (int) (unsigned char) buff->input[i];\n", output);
	iter = patterns;
	while (iter != NULL) {
		transition_pattern(output, iter);
		iter = iter->next;
	}
	fputs(  "if (!match_possible) { break; }\n", output);
	fputs("}\n", output);

	fputs("buff->parsed = last_match;\n", output);
	fputs("switch (last_regex) {\n", output);
	fputs("default: return YEX_ERROR;\n", output);
	run_actions(output, patterns);
	fputs("}\n", output);
	fputs("return YEX_NAME(buff);\n", output);

	fputs("}\n", output);
}

static void transition_pattern(FILE *output, struct pattern_list *pattern) {
	struct dfa *dfa;
	int id;
	long i;
	dfa = pattern->regex;
	id = pattern->id;
	fprintf(output, "switch (state_%d) {\n", id);
	fputs("case -1: break;\n", output);

	for (i = 0; i < dfa->num_nodes; ++i) {
		fprintf(output, "case %ld:\n", i);
		transition_state(output, dfa, i, id);
		fputs("break;\n", output);
	}

	fputs("}\n", output);
}

static void transition_state(FILE *output, struct dfa *dfa, int state, int id) {
	long i;

	fputs("switch (c) {\n", output);
	for (i = 0; i < dfa->num_items; ++i) {
		if (dfa->nodes[state].links[i] == -1) {
			continue;
		}
		fprintf(output, "case %ld:", i);
		fputs("match_possible=1;", output);
		if (dfa->nodes[dfa->nodes[state].links[i]].r) {
			fprintf(output, "last_regex = %d;\n", id);
			fputs("last_match = i+1;\n", output);
			fputs("match_possible = 1;\n", output);
			fprintf(output, "last_regex = %d;\n", id);
		}
		fprintf(output, "state_%d = %dl;\n",
				id, dfa->nodes[state].links[i]);
		fputs("break;\n", output);
	}
	fprintf(output, "default: state_%d = -1l; break;\n", id);
	fputs("}\n", output);
}

static void run_actions(FILE *output, struct pattern_list *pattern) {
	struct pattern_list *iter;

	iter = pattern;
	while (iter != NULL) {
		fprintf(output, "case %d: ", iter->id);
		fputs(iter->code, output);
		fputs("break;", output);
		iter = iter->next;
	}
}
