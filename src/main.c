#include <stdio.h>

#include "nfa.h"

int main(int argc, char **argv) {
	struct arena arena;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s [regex]\n", argv[0]);
		return 1;
	}

	arena_init(&arena);
	printf("%p\n", (void *) nfa_compile(&arena, argv[1]));
	arena_free(&arena);
	return 0;
}
