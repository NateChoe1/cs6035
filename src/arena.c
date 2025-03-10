#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"

/* IMPL-DEF: weird pointer manipulation */

struct arena *arena_new() {
	struct arena *ret;

	ret = xmalloc(sizeof(*ret));
	ret->next = NULL;
	ret->prev = NULL;

	return ret;
}

void *arena_malloc(struct arena *arena, size_t size) {
	struct arena *region;
	region = malloc(size + sizeof(*region));
	if (region == NULL) {
		fputs("arena_malloc() failed\n", stderr);
		exit(EXIT_FAILURE);
	}
	region->prev = arena;
	region->next = arena->next;
	if (arena->next != NULL) {
		arena->next->prev = region;
	}
	arena->next = region;
	return (void *) ((char *) region + sizeof(*region));
}

void *arena_realloc(void *ptr, size_t size) {
	struct arena *region;
	region = (struct arena *) ((char *) ptr - sizeof(*region));
	region = realloc(region, size + sizeof(*region));
	if (region == NULL) {
		fputs("arena_realloc() failed\n", stderr);
		exit(EXIT_FAILURE);
	}
	if (region->prev != NULL) {
		region->prev->next = region;
	}
	if (region->next != NULL) {
		region->next->prev = region;
	}
	return (void *) ((char *) region + sizeof(*region));
}

void arena_freeptr(void *ptr) {
	struct arena *region;
	region = (struct arena *) ((char *) ptr - sizeof(*region));
	if (region->prev != NULL) {
		region->prev->next = region->next;
	}
	if (region->next != NULL) {
		region->next->prev = region->prev;
	}
	free(region);
}

void arena_free(struct arena *arena) {
	struct arena *next;
	while (arena != NULL) {
		next = arena->next;
		free(arena);
		arena = next;
	}
}

void *xmalloc(size_t size) {
	void *ret;
	ret = malloc(size);
	if (ret == NULL) {
		fputs("xmalloc() failed\n", stderr);
		exit(EXIT_FAILURE);
	}
	return ret;
}

void *xrealloc(void *ptr, size_t size) {
	void *ret;
	ret = realloc(ptr, size);
	if (ret == NULL) {
		fputs("xrealloc() failed\n", stderr);
		exit(EXIT_FAILURE);
	}
	return ret;
}
