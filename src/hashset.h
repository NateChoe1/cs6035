#ifndef HASHSET_H
#define HASHSET_H

#include "arena.h"

/* hashsets are just hashmaps with a different name for type safety  */
struct hashset;

struct hashset *hashset_new(struct arena *arena);
void hashset_put(struct hashset *hashset, void *ptr);
int hashset_contains(struct hashset *hashset, void *ptr);
void hashset_remove(struct hashset *hashset, void *ptr);
void hashset_free(struct hashset *hashset);
void hashset_iter(struct hashset *hashset, void *closure,
		void (*callback)(void *closure, void *item));
size_t hashset_size(struct hashset *hashset);

#endif
