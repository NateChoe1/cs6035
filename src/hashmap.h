#ifndef HASHMAP_H
#define HASHMAP_H

#include "arena.h"

struct hashmap_node {
	long key;
	void *value;
	struct hashmap_node *next;
};

struct hashmap {
	struct arena *arena;
	struct hashmap_node **buckets;
	size_t num_buckets;
	size_t num_items;
};

struct hashmap *hashmap_new(struct arena *arena);

void hashmap_put(struct hashmap *hashmap, long key, void *value);
void *hashmap_get(struct hashmap *hashmap, long key);
void hashmap_remove(struct hashmap *hashmap, long key);

void hashmap_iter(struct hashmap *hashmap, void *closure,
		void (*callback)(void *closure, long key, void *value));

#endif
