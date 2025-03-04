#ifndef ARENA_H
#define ARENA_H

struct _arena_region {
	struct _arena_region *next;
};

struct arena {
	struct _arena_region *head;
	struct _arena_region **tail;
};

void arena_init(struct arena *arena);
void *arena_malloc(struct arena *arena, size_t size);
void *arena_realloc(void *ptr, size_t size);
void arena_free(struct arena *arena);

#endif
