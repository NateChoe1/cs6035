#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "getopt.h"
#include "coroutine.h"
#include "stone-parse.h"

static int write_output(char **inputs, FILE *output, int verbose);
static int parse_file(struct stone_parse_state *state,
		FILE *input, FILE *output);

int stone_main(int argc, char **argv) {
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
}

static int write_output(char **inputs, FILE *output, int verbose) {
	int i, ret;
	struct stone_parse_state state;
	FILE *input;

	/* the first input is to initialize the state */
	stone_parse_char(&state, COROUTINE_RESET, output);

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

	if (stone_parse_char(&state, COROUTINE_EOF, output) != -1) {
		fputs("An error occured during parsing.\n", stderr);
		return 1;
	}

	if (output == stdout || !verbose) {
		goto ret;
	}

	fputs("Tokenizer summary:\n", stderr);
	fprintf(stderr, "  Total rules: %ld\n", (long) state.rules_count);

ret:
	return 0;
}

static int parse_file(struct stone_parse_state *state,
		FILE *input, FILE *output) {
	int c;

	for (;;) {
		c = fgetc(input);
		if (c == EOF) {
			return 0;
		}

		if (stone_parse_char(state, c, output) != 0) {
			return 1;
		}
	}
}
