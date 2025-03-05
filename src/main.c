#include <stdio.h>

#include "dfa.h"

int main(int argc, char **argv) {
	struct arena *arena;
	struct dfa *dfa;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s [regex] [string]\n", argv[0]);
		return 1;
	}

	arena = arena_new();
	dfa = dfa_new(arena, argv[1]);
	printf("%ld\n", dfa_check_loose(dfa, argv[2]));
	printf("%ld\n", dfa_check_greedy(dfa, argv[2]));
	arena_free(arena);
	return 0;
}
