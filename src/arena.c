#include <stdio.h>
#include <stdlib.h>

#include "arena.h"

void arena_init(struct arena *arena) {
	arena->head = NULL;
	arena->tail = &arena->head;
}

void *arena_malloc(struct arena *arena, size_t size) {
	struct _arena_region *region;
	region = malloc(size + sizeof(*region));
	if (region == NULL) {
		fputs("arena_malloc() failed\n", stderr);
		exit(EXIT_FAILURE);
	}
	*arena->tail = region;
	arena->tail = &region->next;
	region->next = NULL;
	return (void *) ((char *) region + sizeof(*region));
}

void *arena_realloc(void *ptr, size_t size) {
	struct _arena_region *region;
	region = (struct _arena_region *) ((char *) ptr - sizeof(*region));
	region = realloc(region, size + sizeof(*region));
	if (region == NULL) {
		fputs("arena_realloc() failed\n", stderr);
		exit(EXIT_FAILURE);
	}
	return region;
}

void arena_free(struct arena *arena) {
	struct _arena_region *region, *next;
	region = arena->head;
	while (region != NULL) {
		next = region->next;
		free(region);
		region = next;
	}
}
