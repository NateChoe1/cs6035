#include <stdarg.h>
#include <string.h>

#include "lr.h"
#include "dfa.h"
#include "hashset.h"

struct item {
	struct lr_rule *rule;

	/* in the range [0, rule->prod_len] */
	long position;
	long ahead;
};

struct item_list {
	struct arena *arena;
	struct lr_grammar *grammar;
	struct item *items;
	size_t len;
	size_t alloc;
};

struct long_list {
	long *values;
	size_t len;
	size_t alloc;
};

static struct item_list *get_items(struct arena *arena,
		struct lr_grammar *grammar);
static void add_rule_items(struct item_list *list, struct lr_rule *rule);
static void add_item(struct item_list *list,
		struct lr_rule *rule, long position, long ahead);
static struct dfa *make_dfa(struct arena *arena,
		struct item_list *items, long start_token);

/* dfa methods */
static void enclose(struct state *state, void *arg);
static struct state *step(struct arena *arena, struct state *state, int item,
		void *arg);
static void followups(struct state *state, void *arg, char *ret);
static int get_r(struct state *state, void *arg);

static void enclose_item(long value,
		struct hashset *seen, struct hashset *visited,
		struct long_list *to_visit, struct item_list *items);

static void enclose_tpair(long pair, struct state *state,
		struct hashset *seen, struct hashset *visited,
		struct long_list *to_visit, struct item_list *items);

static void enclose_anont(long token, long ahead, struct state *state,
		struct hashset *seen, struct hashset *visited,
		struct long_list *to_visit, struct item_list *items);

static void init_row(struct arena *arena,
		struct lr_table *table, long state, long start,
		struct item_list *items, struct dfa *dfa);

struct lr_grammar *lr_grammar_new(struct arena *arena,
		long num_terminals, long num_tokens) {
	struct lr_grammar *ret;
	long i, num_nt;

	num_nt = num_tokens - num_terminals;

	ret = arena_malloc(arena, sizeof(*ret));
	ret->arena = arena;
	ret->num_tokens = num_tokens;
	ret->num_terminals = num_terminals;
	ret->rules = arena_malloc(arena, num_nt * sizeof(*ret->rules));

	for (i = 0; i < num_nt; ++i) {
		ret->rules[i] = NULL;
	}

	ret->num_rules = 0;
	ret->rules_alloc = 32;
	ret->rules_linear = arena_malloc(arena,
			ret->rules_alloc * sizeof(*ret->rules_linear));
	return ret;
}

void lr_grammar_add(struct lr_grammar *grammar,
		long product, long *production, long prod_len) {
	struct lr_rule *rule;
	long idx;

	idx = product - grammar->num_terminals;

	rule = arena_malloc(grammar->arena, sizeof(*rule));
	rule->prod = production;
	rule->prod_len = prod_len;
	rule->next = grammar->rules[idx];
	rule->token = product;
	grammar->rules[idx] = rule;

	rule->idx = grammar->num_rules;
	if (grammar->num_rules >= grammar->rules_alloc) {
		grammar->rules_alloc *= 2;
		grammar->rules_linear = arena_realloc(grammar->rules_linear,
				grammar->rules_alloc *
				sizeof(*grammar->rules_linear));
	}
	grammar->rules_linear[grammar->num_rules++] = rule;
}

void lr_grammar_addv(struct lr_grammar *grammar, long product, ...) {
	va_list ap;
	long token, *production;
	long len, alloc;

	va_start(ap, product);

	len = 0;
	alloc = 5;
	production = arena_malloc(grammar->arena, alloc * sizeof(*production));

	for (;;) {
		token = va_arg(ap, long);
		if (token < 0) {
			break;
		}
		if (len >= alloc) {
			alloc *= 2;
			production = arena_realloc(production,
					alloc * sizeof(*production));
		}
		production[len++] = token;
	}

	lr_grammar_add(grammar, product, production, len);
}

struct lr_table *lr_compile(struct arena *arena,
		struct lr_grammar *grammar, long start_token) {
	struct arena *ia, *da;
	struct item_list *items;
	struct dfa *dfa;
	struct lr_table *ret;
	long i;

	ia = arena_new();
	items = get_items(ia, grammar);

	da = arena_new();
	dfa = make_dfa(da, items, start_token);

	ret = arena_malloc(arena, sizeof(*ret));
	ret->arena = arena;
	ret->num_states = dfa->num_nodes;
	ret->num_tokens = grammar->num_tokens;
	ret->table = arena_malloc(arena, ret->num_states * sizeof(*ret->table));

	for (i = 0; i < dfa->num_nodes; ++i) {
		init_row(arena, ret, i, start_token, items, dfa);
	}

	arena_free(ia);
	arena_free(da);

	return ret;
}

static struct item_list *get_items(struct arena *arena,
		struct lr_grammar *grammar) {
	struct item_list *ret;
	long i;

	ret = arena_malloc(arena, sizeof(*ret));
	ret->arena = arena;
	ret->len = 0;
	ret->alloc = 32;
	ret->grammar = grammar;
	ret->items = arena_malloc(arena, ret->alloc * sizeof(*ret->items));

	for (i = 0; i < (long) grammar->num_rules; ++i) {
		add_rule_items(ret, grammar->rules_linear[i]);
	}

	return ret;
}

static void add_rule_items(struct item_list *list, struct lr_rule *rule) {
	long i, j;

	rule->ii = list->len;

	for (i = 0; i < list->grammar->num_terminals; ++i) {
		for (j = 0; j <= (long) rule->prod_len; ++j) {
			add_item(list, rule, j, i);
		}
	}
}

static void add_item(struct item_list *list,
		struct lr_rule *rule, long position, long ahead) {
	if (list->len >= list->alloc) {
		list->alloc *= 2;
		list->items = arena_realloc(list->items,
				list->alloc * sizeof(*list->items));
	}
	list->items[list->len].rule = rule;
	list->items[list->len].position = position;
	list->items[list->len].ahead = ahead;
	++list->len;
}

static struct dfa *make_dfa(struct arena *arena,
		struct item_list *items, long start_token) {
	struct state *initial_state;
	struct lr_rule *start_rule;
	struct lr_grammar *grammar;

	grammar = items->grammar;
	if (start_token < grammar->num_terminals
			|| start_token >= grammar->num_tokens) {
		return NULL;
	}
	start_rule = grammar->rules[start_token - grammar->num_terminals];
	if (start_rule->next != NULL) {
		return NULL;
	}

	initial_state = state_new(arena);
	state_append(initial_state, start_rule->ii);

	return dfa_new(arena, (int) grammar->num_tokens, 1,
			initial_state,
			enclose,
			step,
			followups,
			get_r,
			(void *) items);
}

/* rules for lr(1) parsers:
 *
 * if a state contains item A -> a . B b  c
 * and B -> c is a rule
 * then the state also contains B -> . c  first(bc)
 * */
static void enclose(struct state *state, void *arg) {
	struct arena *arena;
	struct hashset *seen, *visited;
	struct state_item *iter;
	struct item_list *items;
	struct long_list to_visit;
	long i;

	items = (struct item_list *) arg;
	arena = arena_new();
	seen = hashset_new(arena);
	visited = hashset_new(arena);

	to_visit.len = 0;
	to_visit.alloc = 10;
	to_visit.values = arena_malloc(arena,
			to_visit.alloc * sizeof(*to_visit.values));

	iter = state->head;
	while (iter != NULL) {
		enclose_item(iter->value, seen, visited, &to_visit, items);
		iter = iter->next;
	}

	for (i = 0; i < (long) to_visit.len; ++i) {
		enclose_tpair(to_visit.values[i], state,
				seen, visited, &to_visit, items);
	}
	arena_free(arena);
}

static struct state *step(struct arena *arena, struct state *state, int item,
		void *arg) {
	struct state_item *iter;
	struct item_list *items;
	struct state *ret;
	struct lr_rule *rule;
	long position;

	ret = state_new(arena);
	items = (struct item_list *) arg;

	iter = state->head;
	while (iter != NULL) {
		rule = items->items[iter->value].rule;
		position = items->items[iter->value].position;
		if (position >= rule->prod_len) {
			goto skip;
		}
		if (rule->prod[position] != item) {
			goto skip;
		}
		state_append(ret, iter->value+1);
skip:
		iter = iter->next;
	}

	return ret;
}

static void followups(struct state *state, void *arg, char *ret) {
	struct item_list *items;
	struct state_item *iter;
	struct lr_rule *rule;
	long position;

	items = (struct item_list *) arg;

	memset(ret, 0, items->grammar->num_tokens);

	iter = state->head;
	while (iter != NULL) {
		rule = items->items[iter->value].rule;
		position = items->items[iter->value].position;
		if (position >= rule->prod_len) {
			goto skip;
		}
		ret[rule->prod[position]] = 1;
skip:
		iter = iter->next;
	}
}

static int get_r(struct state *state, void *arg) {
	(void) state;
	(void) arg;
	return 0;
}

static void enclose_item(long value,
		struct hashset *seen, struct hashset *visited,
		struct long_list *to_visit, struct item_list *items) {
	struct lr_rule *rule;
	long next, position, ahead;

	hashset_put(seen, value);
	rule = items->items[value].rule;
	position = items->items[value].position;
	ahead = items->items[value].ahead;

	/* corresponds to items like A -> a . */
	if (position >= rule->prod_len) {
		return;
	}

	next = rule->prod[position];
	next *= items->grammar->num_tokens;
	next += (position+1 < rule->prod_len) ? rule->prod[position+1] : ahead;

	if (hashset_contains(visited, next)) {
		return;
	}

	hashset_put(visited, next);
	if (to_visit->len >= to_visit->alloc) {
		to_visit->alloc *= 2;
		to_visit->values = arena_realloc(to_visit->values,
				to_visit->alloc * sizeof(*to_visit->values));
	}
	to_visit->values[to_visit->len++] = next;
}

static void enclose_tpair(long pair, struct state *state,
		struct hashset *seen, struct hashset *visited,
		struct long_list *to_visit, struct item_list *items) {
	struct lr_rule *rule;
	struct lr_grammar *grammar;
	long token, ahead, item;

	grammar = items->grammar;
	token = pair / grammar->num_tokens;
	ahead = pair % grammar->num_tokens;

	if (token < grammar->num_terminals) {
		return;
	}

	if (ahead >= grammar->num_terminals) {
		enclose_anont(token, ahead,
				state, seen, visited, to_visit, items);
		return;
	}

	rule = grammar->rules[token - grammar->num_terminals];

	while (rule != NULL) {
		item = rule->ii;
		item += (rule->prod_len+1) * ahead;

		/* if ahead is n, rule is S->a b, item is S->a b  n where S is
		 * definitely a nonterminal and n is definitely a terminal */

		if (hashset_contains(seen, item)) {
			goto seen;
		}
		state_append(state, item);
		enclose_item(item, seen, visited, to_visit, items);
seen:
		rule = rule->next;
	}
}

/* for some exploratory state a -> b . B c  d, c is a nonterminal. this function
 * calls `explore_tpair(B, first(c))` for all first(c)*/
static void enclose_anont(long token, long ahead, struct state *state,
		struct hashset *seen, struct hashset *visited,
		struct long_list *to_visit, struct item_list *items) {
	struct lr_grammar *grammar;
	struct lr_rule *rule;
	long base, pair;

	grammar = items->grammar;
	base = token * grammar->num_tokens;

	rule = grammar->rules[ahead];
	while (rule != NULL) {
		pair = base + rule->prod[0];
		if (hashset_contains(visited, pair)) {
			goto skip;
		}
		hashset_put(visited, pair);
		enclose_tpair(pair, state, seen, visited, to_visit, items);
skip:
		rule = rule->next;
	}
}

static void init_row(struct arena *arena,
		struct lr_table *table, long state, long start,
		struct item_list *items, struct dfa *dfa) {
	long i;
	struct lr_grammar *grammar;
	struct lr_table_ent *row;
	struct state_item *iter;
	struct item *item;

	grammar = items->grammar;

	row = arena_malloc(arena, table->num_tokens * sizeof(*row));

	/* handle all shift/transition entries
	 * also initialize every entry in this row to an error transition */
	for (i = 0; i < grammar->num_tokens; ++i) {
		if (dfa->nodes[state].links[i] < 0) {
			row[i].type = LR_ERROR;
			continue;
		}
		row[i].type = LR_TRANSITION;
		row[i].value = dfa->nodes[state].links[i];
	}

	/* handle all reduction states
	 * also potentially mark this state as an accept state */
	iter = dfa->nodes[state].state->head;
	while (iter != NULL) {
		item = &items->items[iter->value];

		/* this item corresponds to a shift state */
		if (item->position < item->rule->prod_len) {
			goto skip;
		}

		if (item->rule->token == start) {
			arena_freeptr((void *) row);
			table->table[state] = NULL;
			return;
		}

		i = item->ahead;

		/* reduce/reduce conflict
		 *
		 * TODO: show an error message */
		if (row[i].type == LR_REDUCE) {
			goto skip;
		}

		/* shift/reduce conflict
		 *
		 * TODO: show an error message */
		if (row[i].type == LR_TRANSITION) {
			goto skip;
		}

		row[i].type = LR_REDUCE;
		row[i].value = item->rule->idx;
skip:
		iter = iter->next;
	}

	table->table[state] = row;
}
