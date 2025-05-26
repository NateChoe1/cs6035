#ifndef HASHMAP_H
#define HASHMAP_H

#include "arena.h"

#include "arena.h"

typedef long (*hashmap_hash)(const void *key);
typedef int (*hashmap_eq)(const void *key1, const void *key2);

struct hashmap_node {
	const void *key;
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
void hashmap_put(struct hashmap *hashmap, const void *key, const void *value);
void *hashmap_get(struct hashmap *hashmap, const void *key);
void hashmap_remove(struct hashmap *hashmap, const void *key);

void hashmap_iter(struct hashmap *hashmap, void *closure,
		void (*callback)(void *closure, const void *key, void *value));

#endif
