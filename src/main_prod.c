#include <stdio.h>

#include "lr.h"

#define a 0l
#define b 1l
#define END 2l
#define S1 3l
#define X 4l
#define S 5l

static void print_table(struct lr_table *table, char **tokens);

int main(void) {
	struct arena *ga, *ta;
	struct lr_grammar *grammar;
	struct lr_table *table;
	char *token_names[] = {
		"a",
		"b",
		"$",
		"S",
		"X",
		"S'",
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
