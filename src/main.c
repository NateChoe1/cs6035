#include <stdio.h>

#include "lr.h"

/* basic grammar tokens
 *
 * INT and PLUS are terminals, TERM is a nonterminal
 *
 * three rules:
 *   START -> TERM END
 *   TERM -> INT
 *   TERM -> TERM PLUS INT
 *
 * this makes PLUS a left to right evaluated operation
 * */
#define INT 0l
#define PLUS 1l
#define END 2l
#define TERM 3l
#define START 4l

static void print_table(struct lr_table *table, char **tokens);

int main(void) {
	struct arena *ga, *ta;
	struct lr_grammar *grammar;
	struct lr_table *table;
	char *token_names[] = {
		"I",
		"+",
		"$",
		"T",
		"S",
	};

	ga = arena_new();
	grammar = lr_grammar_new(ga, 3, 5);
	lr_grammar_addv(grammar, TERM, INT, -1l);
	lr_grammar_addv(grammar, TERM, TERM, PLUS, INT, -1l);
	lr_grammar_addv(grammar, START, TERM, END, -1l);

	ta = arena_new();
	table = lr_compile(ta, grammar, START);
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
