#include <stdio.h>

#include "dfa.h"
#include "regex.h"
#include "strmap.h"
#include "getopt.h"

struct parse_state {
	int progress;
	int i;
};

static int write_output(char **inputs, FILE *output, int verbose);
static int parse_file(struct parse_state *state, FILE *input, FILE *output);
static int parse_char(int c, FILE *output, struct parse_state *state);

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
}

static int write_output(char **inputs, FILE *output, int verbose) {
	int i, ret;
	struct parse_state state;
	FILE *input;

	/* the first input is to initialize the state */
	state.progress = 0;
	parse_char(-1, output, &state);

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

	if (parse_char(EOF, output, &state) != 0) {
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

		if (parse_char(c, output, state) != 0) {
			return 1;
		}
	}
}

#define COROUTINE_START switch(state->progress) { \
	case -1: \
		return 1; \
	case 0: \
	((void) 0)
#define COROUTINE_END } do { return 1; } while (0)
#define GET_CHAR do { \
	state->progress = __LINE__; \
	return 0; \
	case __LINE__: \
	(void) 0; \
} while (0)
#define RETURN(value) do { state->progress = -1; return value; } while (0)
static int parse_char(int c, FILE *output, struct parse_state *state) {
	COROUTINE_START;

	(void) output;

	for (state->i = 0; state->i < 100; ++state->i) {
		GET_CHAR;
		printf("%c", c);
	}

	RETURN(0);

	COROUTINE_END;
}
#undef COROUTINE_START
#undef COROUTINE_END
#undef GET_CHAR
