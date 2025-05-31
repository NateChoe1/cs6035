#include <stdio.h>

#include "dfa.h"
#include "file.h"
#include "regex.h"
#include "strmap.h"
#include "getopt.h"

struct pattern_list {
	struct dfa *regex;
	char *code;
	int id;
	int greedy;
	struct pattern_list *next;
};

static struct pattern_list *read_patterns(FILE *input, struct arena *arena);
static void write_autocode(FILE *output, struct pattern_list *patterns);
static void transition_pattern(FILE *output, struct pattern_list *pattern);
static void transition_state(FILE *output, struct dfa *dfa, int state,
		int id, int is_greedy);
void print_match_code(FILE *output, struct dfa *dfa, int dst,
		int id, int is_greedy);
static void run_actions(FILE *output, struct pattern_list *pattern);

int yex_main(int argc, char **argv) {
	FILE *input, *output;
	struct arena *ra;
	struct pattern_list *patterns;
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
			goto unknown_arg;
		case -1:
			goto got_args;
		}
	}
got_args:

	printf("%d %d\n", to_stdout, verbose);
	return 0;

	input = fopen(argv[1], "r");
	output = fopen(argv[2], "w");

	if (file_copysection(input, output) != 0) {
		fputs("Input file doesn't contain a header!\n", stderr);
		fclose(input);
		fclose(output);
		return 1;
	}
	ra = arena_new();
	patterns = read_patterns(input, ra);

	write_autocode(output, patterns);

	fclose(input);
	fclose(output);
	return 0;
unknown_arg:
	fprintf(stderr, "Usage: %s [-t] [-n|-v] [file...]\n",
			argv[0]);
	return 1;
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
		pattern = file_readline(input, tmp);
		if (pattern == NULL || pattern[0] == '\0') {
			break;
		}
		code = file_readline(input, arena);
		if (code == NULL) {
			break;
		}
		cur = arena_malloc(arena, sizeof(*cur));
		cur->regex = (struct dfa *) regex_compile(arena, pattern+1);
		cur->greedy = pattern[0] == '!';
		cur->code = code;
		cur->next = ret;
		cur->id = i++;
		ret = cur;
	}

	arena_free(tmp);

	return ret;
}

/* it is possible to do all of this parsing in O(n) time by unifying all of our
 * dfas into a single dfa, but that would use a ridiculous amount of memory. */
static void write_autocode(FILE *output, struct pattern_list *patterns) {
	struct pattern_list *iter;

	fputs("#include <yex.h>\n", output);
	fputs("#ifndef YEX_NAME\n", output);
	fputs("#define YEX_NAME yex_read\n", output);
	fputs("#endif\n", output);
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

	fputs("buff->t_start = buff->parsed;\n", output);

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
		transition_state(output, dfa, i, id, pattern->greedy);
		fputs("break;\n", output);
	}

	fputs("}\n", output);
}

static void transition_state(FILE *output, struct dfa *dfa, int state,
		int id, int is_greedy) {
	long i;

	fputs("switch (c) {\n", output);
	for (i = 0; i < dfa->num_items; ++i) {
		if (dfa->nodes[state].links[i] == -1) {
			continue;
		}
		fprintf(output, "case %ld:", i);
		fputs("match_possible=1;", output);
		print_match_code(output, dfa, dfa->nodes[state].links[i],
				id, is_greedy);
		fputs("break;\n", output);
	}
	fprintf(output, "default: state_%d = -1l; break;\n", id);
	fputs("}\n", output);
}

void print_match_code(FILE *output, struct dfa *dfa, int dst,
		int id, int is_greedy) {
	if (!dfa->nodes[dst].r) {
		return;
	}
	fprintf(output, "last_regex = %d;\n", id);
	fputs("last_match = i+1;\n", output);
	fprintf(output, "last_regex = %d;\n", id);
	fprintf(output, "state_%d = %dl;\n", id, is_greedy ? dst : -1);
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
