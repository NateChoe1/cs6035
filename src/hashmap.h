#ifndef HASHMAP_H
#define HASHMAP_H

#include "arena.h"

#include "arena.h"

typedef long (*hashmap_hash)(void *key);
typedef int (*hashmap_eq)(void *key1, void *key2);

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

	hashmap_hash hash;
	hashmap_eq equals;
};

struct hashmap *hashmap_new(struct arena *arena,
		hashmap_hash hash, hashmap_eq equals);
void hashmap_put(struct hashmap *hashmap, void *key, void *value);
void *hashmap_get(struct hashmap *hashmap, void *key);
void hashmap_remove(struct hashmap *hashmap, void *key);

void hashmap_iter(struct hashmap *hashmap, void *closure,
		void (*callback)(void *closure, void *key, void *value));

#endif
