#include <ctype.h>
#include <stdlib.h>

#include "lr.h"
#include "file.h"
#include "lacc.h"

struct spec {
	long num_terminals;
	long num_tokens;
	char **on_read;
	long start_token;
	long eof_token;
	struct lr_grammar *grammar;
	struct lr_table *table;
	char **on_reduce;
};

static struct spec *read_spec(FILE *input, struct arena *arena);
static void write_spec(struct arena *arena, struct spec *spec, FILE *output);
static void print_transitions(struct spec *spec, FILE *output);
static void print_state(struct spec *spec, long state, FILE *output);
static int get_tcount(FILE *input, long *num_terminals, long *num_tokens);
static long *parse_list(FILE *input, struct arena *arena);
static long atoln(char *ptr, long n);

int lacc_main(int argc, char **argv) {
	FILE *input, *output;
	struct arena *arena;
	struct spec *spec;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s [input.lacc] [output.c]\n", argv[0]);
		return 1;
	}

	input = fopen(argv[1], "r");
	output = fopen(argv[2], "w");
	arena = arena_new();
	
	file_copysection(input, output);

	spec = read_spec(input, arena);
	if (spec == NULL) {
		fputs("Failed to read spec\n", stderr);
		fclose(input);
		fclose(output);
		arena_free(arena);
		return 1;
	}

	write_spec(arena, spec, output);

	arena_free(arena);
	fclose(input);
	fclose(output);
	return 0;
}

static struct spec *read_spec(FILE *input, struct arena *arena) {
	struct spec *ret;
	long i, *l;
	char *s;
	size_t rl, ra;

	ret = arena_malloc(arena, sizeof(*ret));
	if (get_tcount(input, &ret->num_terminals, &ret->num_tokens)) {
		fputs("Failed to get token count\n", stderr);
		return NULL;
	}

	ret->on_read = arena_malloc(arena,
			ret->num_tokens * sizeof(*ret->on_read));

	for (i = 0; i < ret->num_terminals; ++i) {
		ret->on_read[i] = file_readline(input, arena);
		if (ret->on_read[i] == NULL) {
			fputs("Failed to read lex hook\n", stderr);
			return NULL;
		}
	}

	l = parse_list(input, arena);
	if (l == NULL || l[0] == -1 || l[1] == -1) {
		fputs("Failed to read start token\n", stderr);
		return NULL;
	}
	ret->start_token = l[0];
	ret->eof_token = l[1];
	arena_freeptr(l);

	rl = 0;
	ra = 32;
	ret->on_reduce = arena_malloc(arena, ra * sizeof(*ret->on_reduce));
	ret->grammar = lr_grammar_new(arena,
			ret->num_terminals, ret->num_tokens);
	for (;;) {
		l = parse_list(input, arena);

		if (l == NULL) {
			break;
		}

		s = file_readline(input, arena);

		if (s == NULL) {
			fputs("Production has no associated reduction code!\n",
					stderr);
			return NULL;
		}

		for (i = 0; l[i+1] != -1; ++i) ;
		if (i == 0) {
			fputs("Production only has a product\n", stderr);
			return NULL;
		}

		if (rl >= ra) {
			ra *= 2;
			ret->on_reduce = arena_realloc(ret->on_reduce,
					ra * sizeof(*ret->on_reduce));
		}

		ret->on_reduce[rl++] = s;
		lr_grammar_add(ret->grammar, l[0], l+1, i);
	}

	return ret;
}

static void write_spec(struct arena *arena, struct spec *spec, FILE *output) {
	spec->table = lr_compile(arena, spec->grammar, spec->start_token);

	fputs("#ifdef LACC_V\n", output);
	fputs("#undef LACC_V\n", output);
	fputs("#endif\n", output);
	fputs("#ifndef LACC_TYPE\n", output);
	fputs("#define LACC_TYPE int\n", output);
	fputs("#undef LACC_ERROR\n", output);
	fputs("#define LACC_ERROR -1\n", output);
	fputs("#endif\n", output);
	fputs("#ifndef LACC_NAME\n", output);
	fputs("#define LACC_NAME yyparse\n", output);
	fputs("#endif\n", output);
	fputs("#ifndef LACC_STACKSIZE\n", output);
	fputs("#define LACC_STACKSIZE 200\n", output);
	fputs("#endif\n", output);

	fputs("struct lacc_stacki {\n", output);
	fputs(  "int token;\n", output);
	fputs(  "long state;\n", output);
	fputs(  "LACC_TYPE value;\n", output);
	fputs("};\n", output);

	fputs("LACC_TYPE LACC_NAME(int (*lacc_gett)(LACC_CLOSURE closure), "
		"LACC_CLOSURE lacc_closure) {\n", output);

	fputs("long lacc_slen;\n", output);
	fputs("int lacc_ahead, lacc_trans;\n", output);
	fputs("struct lacc_stacki lacc_stack[LACC_STACKSIZE];\n", output);
	fputs("LACC_TYPE lacc_v;\n", output);

	fputs("#define LACC_V(n) (lacc_stack[lacc_slen+n].value)\n", output);

	fputs("lacc_ahead = lacc_gett(lacc_closure);\n", output);
	fputs("if (lacc_ahead < 0) {\n", output);
	fputs(  "return LACC_ERROR;\n", output);
	fputs("}\n", output);

	fputs("lacc_stack[0].state = 0;\n", output);
	fputs("lacc_slen = 1;\n", output);

	fputs("for (;;) {\n", output);
	fputs(  "lacc_trans = lacc_ahead;\n", output);
	fputs("lacc_skip:\n", output);
	fputs(  "if (lacc_slen < 1 || lacc_slen >= LACC_STACKSIZE) {\n",
			output);
	fputs(    "return LACC_ERROR;\n", output);
	fputs(  "}\n", output);
	print_transitions(spec, output);
	fputs(  "lacc_ahead = lacc_gett(lacc_closure);\n", output);
	fputs(  "if (lacc_ahead == -1) {\n", output);
	fprintf(output,"lacc_ahead = %ld;\n", spec->eof_token);
	fputs(  "}\n", output);
	fputs(  "if (lacc_ahead < -1) {\n", output);
	fputs(    "return LACC_ERROR;\n", output);
	fputs(  "}\n", output);
	fputs("}\n", output);

	fputs("}\n", output);
}

static void print_transitions(struct spec *spec, FILE *output) {
	long i;
	fputs("switch (lacc_stack[lacc_slen-1].state) {\n", output);
	for (i = 0; i < spec->table->num_states; ++i) {
		fprintf(output, "case %ld:\n", i);
		print_state(spec, i, output);
		fputs("break;\n", output);
	}
	fputs("default: return LACC_ERROR;\n", output);
	fputs("}\n", output);
}

static void print_state(struct spec *spec, long state, FILE *output) {
	long i, r;
	struct lr_grammar *grammar;
	struct lr_table *table;

	grammar = spec->grammar;
	table = spec->table;

	if (table->table[state] == NULL) {
		fputs("return lacc_stack[1].value;", output);
		return;
	}

	fputs("switch (lacc_trans) {\n", output);
	for (i = 0; i < table->num_terminals; ++i) {
		if (table->table[state][i].type == LR_ERROR) {
			continue;
		}
		fprintf(output, "case %ld:\n", i);
		if (table->table[state][i].type == LR_TRANSITION) {
			fputs(spec->on_read[i], output);
			fprintf(output, "lacc_stack[lacc_slen].token = %ld;\n",
					i);
			fprintf(output, "lacc_stack[lacc_slen].state = %ld;\n",
					table->table[state][i].value);
			fputs("lacc_stack[lacc_slen].value = lacc_v;\n",
					output);
			fputs("++lacc_slen;\n", output);
			fputs("break;\n", output);
			continue;
		}
		/* reduction */

		r = table->table[state][i].value;
		fprintf(output, "lacc_slen -= %ld;\n",
				grammar->rules_linear[r]->prod_len);
		fprintf(output, "lacc_trans = %ld;\n",
				grammar->rules_linear[r]->token);
		fputs(spec->on_reduce[r], output);
		fputs("goto lacc_skip;\n", output);
	}
	for (i = table->num_terminals; i < table->num_tokens; ++i) {
		if (table->table[state][i].type != LR_TRANSITION) {
			continue;
		}
		fprintf(output, "case %ld:\n", i);
		fprintf(output, "lacc_stack[lacc_slen].token = %ld;\n", i);
		fprintf(output, "lacc_stack[lacc_slen].state = %ld;\n",
				table->table[state][i].value);
		fputs("lacc_stack[lacc_slen].value = lacc_v;\n", output);
		fputs("++lacc_slen;\n", output);
		fputs("continue;\n", output);
	}
	fputs("default: return LACC_ERROR;\n", output);
	fputs("}\n", output);
}

static int get_tcount(FILE *input, long *num_terminals, long *num_tokens) {
	struct arena *arena;
	long *list;

	arena = arena_new();
	list = parse_list(input, arena);
	if (list[0] == -1 || list[1] == -1 || list[2] != -1) {
		arena_free(arena);
		return 1;
	}
	*num_terminals = list[0];
	*num_tokens = list[1];

	arena_free(arena);
	return 0;
}

static long *parse_list(FILE *input, struct arena *arena) {
	struct arena *tmp;
	long i, *ret;
	char *line;
	size_t seen, len;

	tmp = arena_new();
	line = file_readline(input, tmp);
	if (line == NULL) {
		arena_free(tmp);
		return NULL;
	}

	len = 2;
	for (i = 0; line[i] != '\0'; ++i) {
		if (line[i] == ' ') {
			++len;
		}
	}
	ret = arena_malloc(arena, len * sizeof(*ret));
	ret[len-1] = -1;

	seen = len = 0;
	for (i = 0; line[i] != '\0'; ++i) {
		if (line[i] == ' ') {
			ret[seen++] = atoln(line+len, i-len);
			len = i+1;
		}
	}
	ret[seen] = atol(line+len);

	arena_free(tmp);
	return ret;
}

static long atoln(char *ptr, long n) {
	long i, ret;
	ret = 0;
	for (i = 0; i < n && ptr[i] != '\0'; ++i) {
		if (!isdigit(ptr[i])) {
			break;
		}
		ret *= 10;
		ret += ptr[i] - '0';
	}
	return ret;
}
