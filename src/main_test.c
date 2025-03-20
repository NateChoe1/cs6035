#include <stdio.h>

#include "lr.h"
#include "regex.h"

static int regex_matches(char *regex, char *str);
static void test_lalr(void);

static void t_assert(int test, char *exp, char *file, int line);
static void summarize(void);
#define assert(v) t_assert((v) != 0, #v, __FILE__, __LINE__)
#define LEN(arr) (sizeof(arr) / sizeof(*arr))

int main() {
	assert(regex_matches("a*b", "aaaab"));
	assert(!regex_matches("a*b", "aaaac"));
	assert(regex_matches("ca*t", "caaat"));
	assert(!regex_matches("ca*t", "caabt"));
	assert(regex_matches("[a-zA-Z]*a", "ajkAJHa"));
	assert(!regex_matches("[a-zA-Z]*a", "ajkA1Ha"));
	assert(!regex_matches("[a-zA-Z]*a", "ajkAJHb"));
	assert(regex_matches("cat|dog", "cat"));
	assert(regex_matches("cat|dog", "dog"));
	assert(!regex_matches("cat|dog", "bug"));

	test_lalr();

	summarize();
	return 0;
}

static void test_lalr(void) {
	struct arena *arena;
	struct lr_table *table;
	struct lr_grammar *grammar;
	unsigned long i, j;

	/* this grammar was taken from
	 *
	 * https://suif.stanford.edu/dragonbook/lecture-notes/Stanford-CS143/11-LALR-Parsing.pdf
	 * 
	 * it's an example of a grammar which can be parsed with an lr(1)
	 * parser, but which is more efficient to parse with an lalr(1) parser
	 * */
#define a 0l
#define b 1l
#define end 2l
#define S_ 3l
#define X 4l
#define S 5l

#define accept -1000l
#define error -1001l

	/* positive numbers are transitions, negative numbers are reductions
	 *
	 * luckily, we will never have a transition to or reduction by 0, since
	 * reductions by 0 are just accept states and transitions to 0 would
	 * imply that the start token isn't really the start token.
	 *
state	a	b	$	S	X	S'	
0	T/1	T/2	ERROR	T/3	T/4	ERROR	
1	T/1	T/2	ERROR	ERROR	T/5	ERROR	
2	R/3	R/3	R/3	ERROR	ERROR	ERROR	
3	ERROR	ERROR	T/6	ERROR	ERROR	ERROR	
4	T/1	T/2	ERROR	ERROR	T/7	ERROR	
5	R/2	R/2	ERROR	ERROR	ERROR	ERROR	
6	ACCEPT
7	ERROR	ERROR	R/1	ERROR	ERROR	ERROR
	 * */
	const long expected[8][6] = {
		{1, 2, error, 3, 4, error},
		{1, 2, error, error, 5, error},
		{-3, -3, -3, error, error, error},
		{error, error, 6, error, error, error},
		{1, 2, error, error, 7, error},
		{-2, -2, -2, error, error, error},
		{accept, accept, accept, accept, accept},
		{error, error, -1, error, error, error},
	};

	arena = arena_new();
	grammar = lr_grammar_new(arena, end+1, S+1);
	lr_grammar_addv(grammar, S, S_, end, -1l);
	lr_grammar_addv(grammar, S_, X, X, -1l);
	lr_grammar_addv(grammar, X, a, X, -1l);
	lr_grammar_addv(grammar, X, b, -1l);
	table = lr_compile(arena, grammar, S);

#undef a
#undef b
#undef end
#undef S_
#undef X
#undef S

	assert(LEN(expected) == table->num_states);
	if (LEN(expected) != table->num_states) {
		goto end;
	}
	for (i = 0; i < LEN(expected); ++i) {
		if (table->table[i] == NULL) {
			assert(expected[i][0] == accept);
			continue;
		}
		for (j = 0; j < LEN(expected[i]); ++j) {
			if (expected[i][j] == error) {
				assert(table->table[i][j].type == LR_ERROR);
				continue;
			}
			if (expected[i][j] >= 0) {
				assert(table->table[i][j].type == LR_TRANSITION);
				assert(table->table[i][j].value == expected[i][j]);
				continue;
			}
			assert(table->table[i][j].type == LR_REDUCE);
			assert(table->table[i][j].value == -expected[i][j]);
		}
	}

#undef accept
#undef error

end:
	arena_free(arena);
}

static int regex_matches(char *regex, char *str) {
	struct regex *compiled;
	struct arena *arena;
	long mlen;

	arena = arena_new();
	compiled = regex_compile(arena, regex);
	mlen = regex_greedy_match(compiled, str);
	arena_free(arena);
	return mlen >= 0 && str[mlen] == '\0';
}

static int passed = 0, total = 0;

static void t_assert(int test, char *exp, char *file, int line) {
	++total;
	if (test) {
		++passed;
		return;
	}
	fprintf(stderr, "[FAIL]\tAssertion failed: %s (%s:%d)\n",
			exp, file, line);
}

static void summarize() {
	fprintf(stderr, "[INFO]\tFinal results: %d/%d tests passed\n",
			passed, total);
	if (passed < total) {
		fputs("[FAIL]\tSome tests failed\n", stderr);
	} else {
		fputs("[SUCCESS]\tCongratulations, you don't suck at coding\n",
				stderr);
	}
	fputs("[INFO]\tRemember to run this test with Valgrind\n", stderr);
}
