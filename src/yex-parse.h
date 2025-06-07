#ifndef YEX_PARSE_H
#define YEX_PARSE_H

#include <stdio.h>

#include "arena.h"

struct yex_parse_state {
	/* progress state */
	int parse_char_progress;
	int parse_definitions_progress;
	int parse_rules_progress;
	int parse_rules_closed_progress;

	/* parse_definitions state */
	int in_def_block;

	/* parse_rules state */
	char pr_c[4];
	int pr_i;
	int pr_l;

	/* global state */
	long i;
	int ret;
	int eof;
	char line[512];
	unsigned long line_len;
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

/* this is a coroutine, see coroutine.h */
int yex_parse_char(struct yex_parse_state *state, int c, FILE *output);

#endif
