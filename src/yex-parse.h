#ifndef YEX_PARSE_H
#define YEX_PARSE_H

#include <stdio.h>

#include "sb.h"
#include "arena.h"
#include "regex.h"

struct yex_parse_rule {
	char *re;       /* the regex itself */
	char *trail;    /* the trailing context. for example, if the regex ends
			   with '$', then this is "\n". if there is no trailing
			   context, then this is NULL. */
	int anchored;   /* 1 if the regex starts with '^' */

	char *action;

	struct regex *re_dfa;
	struct regex *trail_dfa;

	char *states;
};

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

	/* parse_rules_closed state */
	struct sb *sb;
	long bc;

	/* global state */
	size_t i, j;
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
	size_t sh_states_count;
	size_t sh_states_alloc;

	/* exclusive states, defined with %x*/
	char **ex_states;
	size_t ex_states_count;
	size_t ex_states_alloc;

	/* rules */
	struct yex_parse_rule **rules;
	size_t rules_count;
	size_t rules_alloc;
};

/* this is a coroutine, see coroutine.h */
int yex_parse_char(struct yex_parse_state *state, int c, FILE *output);

#endif
