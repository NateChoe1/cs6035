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

int main(void) {
	struct arena *ga, *ta;
	struct lr_grammar *grammar;
	struct lr_table *table;

	ga = arena_new();
	grammar = lr_grammar_new(ga, 3, 5);
	lr_grammar_addv(grammar, TERM, INT, -1l);
	lr_grammar_addv(grammar, TERM, TERM, PLUS, INT, -1l);
	lr_grammar_addv(grammar, START, TERM, END, -1l);

	ta = arena_new();
	table = lr_compile(ta, grammar, START);
	arena_free(ga);

	(void) table;

	arena_free(ta);
	return 0;
}
