#include <stdio.h>

#include "regex.h"

static int regex_matches(char *regex, char *str);

static void t_assert(int test, char *exp, char *file, int line);
static void summarize();
#define assert(v) t_assert((v) != 0, #v, __FILE__, __LINE__)

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

	summarize();
	return 0;
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
