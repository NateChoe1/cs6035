#include <stdio.h>

#include "lr.h"

#define a 0l
#define b 1l
#define END 2l
#define X 3l
#define S1 4l
#define S 5l

static void print_table(struct lr_table *table, char **tokens);

/* 0:	S -> . S1 $	a
 * 1:	S -> S1 . $	a
 * 2:	S -> S1 $ .	a
 * 3:	S -> . S1 $	b
 * 4:	S -> S1 . $	b
 * 5:	S -> S1 $ .	b
 * 6:	S -> . S1 $	$
 * 7:	S -> S1 . $	$
 * 8:	S -> S1 $ .	$
 * 9:	S1 -> . X X	a
 * 10:	S1 -> X . X	a
 * 11:	S1 -> X X .	a
 * 12:	S1 -> . X X	b
 * 13:	S1 -> X . X	b
 * 14:	S1 -> X X .	b
 * 15:	S1 -> . X X	$
 * 16:	S1 -> X . X	$
 * 17:	S1 -> X X .	$
 * 18:	X -> . a X	a
 * 19:	X -> a . X	a
 * 20:	X -> a X .	a
 * 21:	X -> . a X	b
 * 22:	X -> a . X	b
 * 23:	X -> a X .	b
 * 24:	X -> . a X	$
 * 25:	X -> a . X	$
 * 26:	X -> a X .	$
 * 27:	X -> . b	a
 * 28:	X -> b .	a
 * 29:	X -> . b	b
 * 30:	X -> b .	b
 * 31:	X -> . b	$
 * 32:	X -> b .	$
 * */

int main(void) {
	struct arena *ga, *ta;
	struct lr_grammar *grammar;
	struct lr_table *table;
	char *token_names[] = {
		"a",
		"b",
		"$",
		"X",
		"S'",
		"S",
	};

	ga = arena_new();
	grammar = lr_grammar_new(ga, END+1, S+1);
	lr_grammar_addv(grammar, S, S1, END, -1l);
	lr_grammar_addv(grammar, S1, X, X, -1l);
	lr_grammar_addv(grammar, X, a, X, -1l);
	lr_grammar_addv(grammar, X, b, -1l);

	ta = arena_new();
	table = lr_compile(ta, grammar, S);
	arena_free(ga);

	print_table(table, token_names);

	arena_free(ta);
	return 0;
}

static void print_table(struct lr_table *table, char **tokens) {
	long i, j;

	/* print table header */
	fputs("state\t", stdout);
	for (i = 0; i < table->num_tokens; ++i) {
		fputs(tokens[i], stdout);
		putchar('\t');
	}
	putchar('\n');

	/* print table contents */
	for (i = 0; i < table->num_states; ++i) {
		printf("%ld\t", i);
		if (table->table[i] == NULL) {
			puts("ACCEPT");
			continue;
		}
		for (j = 0; j < table->num_tokens; ++j) {
			switch (table->table[i][j].type) {
			case LR_ERROR:
				fputs("ERROR\t", stdout);
				continue;
			case LR_TRANSITION:
				fputs("T/", stdout);
				break;
			case LR_REDUCE:
				fputs("R/", stdout);
			}
			printf("%ld\t", table->table[i][j].value);
		}
		putchar('\n');
	}
}
