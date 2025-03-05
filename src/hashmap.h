#ifndef HASHMAP_H
#define HASHMAP_H

#include "arena.h"

struct hashmap_node {
	void *key;
	void *value;
	struct hashmap_node *next;
};

struct hashmap {
	struct arena *arena;
	struct hashmap_node **buckets;
	size_t num_buckets;
	size_t num_items;
};

/* if `arena` is NULL, a new arena will be allocated specifically for this
 * hashmap */
struct hashmap *hashmap_new(struct arena *arena);

void hashmap_put(struct hashmap *hashmap, void *key, void *value);
void *hashmap_get(struct hashmap *hashmap, void *key);
void hashmap_remove(struct hashmap *hashmap, void *key);

/* only use this function if you used NULL as the arena to create the hashmap in
 * the first place*/
void hashmap_free(struct hashmap *hashmap);

void hashmap_iter(struct hashmap *hashmap, void *closure,
		void (*callback)(void *closure, void *key, void *value));

#endif
