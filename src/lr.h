#ifndef LR_H
#define LR_H

#include "arena.h"

/* this code will only implement lr(1) parsers since i'm too lazy to implement
 * multi-token lookahead and build all the data structures to handle that. */

struct lr_rule {
	long token;
	long *prod;
	long prod_len;

	/* contains an index into `rules_linear` */
	long idx;

	/* "item index", index of the first item that uses this rule. i really
	 * don't like this field, because it's only used by lr_compile, and
	 * can't be initialized while we're building the grammar. */
	long ii;

	/* contains a pointer to the next rule that produces the same token as
	 * this one */
	struct lr_rule *next;
};

/* RULES:
 *
 * the first `num_terminals` tokens are terminals
 *
 * the next `num_tokens-num_terminals` tokens are nonterminals
 *
 * the production rules for token `n` is in `rules[n-num_terminals]`
 *
 * `rules_linear` contains the same set of rules as `rules`
 *
 * by definition, terminals have no production rules
 * */
struct lr_grammar {
	struct arena *arena;

	long num_tokens;
	long num_terminals;

	struct lr_rule **rules;

	size_t num_rules;
	size_t rules_alloc;
	struct lr_rule **rules_linear;
};

enum lr_rtype {
	/* equivalent to both shift and goto in the cs6035 slides */
	LR_TRANSITION,

	LR_REDUCE,
	LR_ERROR
};

struct lr_table_ent {
	enum lr_rtype type;
	long value;
};

struct lr_table {
	struct arena *arena;
	long num_states;
	long num_tokens;
	long num_terminals;

	/* table[state][token]
	 *
	 * this is why i'm only handling the lr(1) case, otherwise we would need
	 * some ridiculous table[state][t1,t2,...,tn] structure and i'm way too
	 * lazy to implement that.
	 *
	 * if table[state] == NULL, then state is an accept state.
	 * */
	struct lr_table_ent **table;
};

struct lr_grammar *lr_grammar_new(struct arena *arena,
		long num_terminals, long num_tokens);

/* `production` should be allocated with the same arena as `grammar` */
void lr_grammar_add(struct lr_grammar *grammar,
		long product, long *production, long prod_len);

/* the production 5 -> 1 2 3 is created with
 *
 *   lr_grammar_addv(grammar, 5l, 1l, 2l, 3l, -1l)
 *
 * */
void lr_grammar_addv(struct lr_grammar *grammar, long product, ...);

struct lr_table *lr_compile(struct arena *arena,
		struct lr_grammar *grammar, long start_token);

#endif
