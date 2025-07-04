#include "arena.h"
#include "regex.h"

#include "main_test.h"
#include "regex_test.h"

static int regex_matches(char *regex, char *str);

void test_regex(void) {
	assert(regex_matches("a*b", "aaaab"));
	assert(!regex_matches("a*b", "aaaac"));
	assert(regex_matches("ca*t", "caaat"));
	assert(!regex_matches("ca*t", "caabt"));
	assert(regex_matches("[a-zA-Z]*a", "ajkAZJHa"));
	assert(!regex_matches("[a-zA-Z]*a", "ajkZA1Ha"));
	assert(!regex_matches("[a-zA-Z]*a", "ajkZAJHb"));
	assert(regex_matches("cat|dog", "cat"));
	assert(regex_matches("cat|dog", "dog"));
	assert(!regex_matches("cat|dog", "bug"));
	assert(regex_matches("[:alnum:]*", "abcd1234"));
	assert(!regex_matches("[:alnum:]*", "hello."));
	assert(regex_matches("[.xdigit.]*", "abcd1234"));
	assert(!regex_matches("[=xdigit=]*", "zzz"));
	assert(!regex_matches("[!xdigit!]*", "abcd1234"));
	assert(regex_matches("a.c", "abc"));
	assert(!regex_matches("a.c", "a\nc"));
	assert(!regex_matches("(ab*)*", "b"));
	assert(regex_matches("(ab*)*", "abbbabbbaabbb"));
	assert(regex_matches("a+b", "aaab"));
	assert(!regex_matches("a+b", "b"));
	assert(!regex_matches("a{2,4}", "a"));
	assert(regex_matches("a{2,4}", "aa"));
	assert(regex_matches("a{2,4}", "aaa"));
	assert(regex_matches("a{2,4}", "aaaa"));
	assert(!regex_matches("a{2,4}", "aaaaa"));
	assert(regex_matches("(ab){2,4}", "ababab"));
	assert(regex_matches("a{1,}", "a"));
	assert(regex_matches("a{1,}", "aaa"));
	assert(regex_matches("a{0,}", ""));
	assert(!regex_matches("a{4,}", "aaa"));
	assert(regex_matches("ab?", "a"));
	assert(regex_matches("ab?", "ab"));
	assert(!regex_matches("ab?", "ac"));
	assert(regex_matches("[^abc]", "z"));
	assert(!regex_matches("[^abc]", "a"));
	assert(regex_matches("", ""));
	assert(!regex_matches("", "a"));
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
