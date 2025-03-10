#ifndef HASHSET_H
#define HASHSET_H

#include "arena.h"

/* hashsets are just hashmaps with a different name for type safety  */
struct hashset;

struct hashset *hashset_new(struct arena *arena);
void hashset_put(struct hashset *hashset, long value);
int hashset_contains(struct hashset *hashset, long value);
void hashset_remove(struct hashset *hashset, long value);
void hashset_iter(struct hashset *hashset, void *closure,
		void (*callback)(void *closure, long item));
size_t hashset_size(struct hashset *hashset);

#endif
