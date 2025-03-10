#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

struct arena {
	struct arena *next;
	struct arena *prev;
};

struct arena *arena_new();
void *arena_malloc(struct arena *arena, size_t size);
void *arena_realloc(void *ptr, size_t size);
void arena_freeptr(void *ptr);
void arena_free(struct arena *arena);

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);

#endif
