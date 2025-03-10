#include <stdio.h>

#include "regex.h"

int main(int argc, char **argv) {
	struct arena *arena;
	struct regex *regex;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s [regex] [string]\n", argv[0]);
		return 1;
	}

	arena = arena_new();
	regex = regex_compile(arena, argv[1]);
	if (regex == NULL) {
		fprintf(stderr, "regex %s failed to compile\n", argv[1]);
		return 1;
	}
	printf("%ld\n", regex_nongreedy_match(regex, argv[2]));
	printf("%ld\n", regex_greedy_match(regex, argv[2]));
	arena_free(arena);
	return 0;
}
