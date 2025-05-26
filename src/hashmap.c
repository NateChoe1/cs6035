#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

static size_t next_bucket(size_t n);
static void reshuffle(struct hashmap *hashmap);
static void insert(struct hashmap *hashmap, struct hashmap_node *node);
struct hashmap_node **get(struct hashmap *hashmap, void *key);

#define INITIAL_BUCKETS 31

struct hashmap *hashmap_new(struct arena *arena,
		hashmap_hash hash, hashmap_eq equals) {
	struct hashmap *ret;
	size_t i;

	ret = arena_malloc(arena, sizeof(*ret));
	ret->arena = arena;
	ret->num_buckets = INITIAL_BUCKETS;
	ret->buckets = arena_malloc(arena,
			ret->num_buckets * sizeof(*ret->buckets));
	for (i = 0; i < ret->num_buckets; ++i) {
		ret->buckets[i] = NULL;
	}
	ret->num_items = 0;
	ret->hash = hash;
	ret->equals = equals;
	return ret;
}

void hashmap_put(struct hashmap *hashmap, void *key, void *value) {
	struct hashmap_node *node;

	node = *get(hashmap, key);
	if (node != NULL) {
		node->value = value;
		return;
	}

	if ((hashmap->num_items++) / hashmap->num_buckets >= 2) {
		reshuffle(hashmap);
	}

	node = arena_malloc(hashmap->arena, sizeof(*node));
	node->key = key;
	node->value = value;
	insert(hashmap, node);
}

void *hashmap_get(struct hashmap *hashmap, void *key) {
	struct hashmap_node *node;

	node = *get(hashmap, key);
	if (node == NULL) {
		return NULL;
	}
	return node->value;
}

void hashmap_remove(struct hashmap *hashmap, void *key) {
	struct hashmap_node **node, *old;
	node = get(hashmap, key);
	if (*node == NULL) {
		return;
	}
	old = *node;
	*node = (*node)->next;
	arena_freeptr(old);
}

/* obviously it would be ideal if we could always return prime numbers, but for
 * word aligned pointer addresses i think this function is fine, especially
 * considering the fact that a lot of the numbers we pick will just happen to
 * be mersenne primes. */
static size_t next_bucket(size_t n) {
	return n*2+1;
}

static void reshuffle(struct hashmap *hashmap) {
	size_t i, old_bcount, new_bcount;
	struct hashmap_node *iter, *next, **old_buckets;

	old_bcount = hashmap->num_buckets;
	new_bcount = next_bucket(old_bcount);
	hashmap->num_buckets = new_bcount;

	old_buckets = xmalloc(old_bcount * sizeof(*old_buckets));
	memcpy(old_buckets, hashmap->buckets,
			old_bcount * sizeof(*old_buckets));

	hashmap->buckets = arena_realloc(hashmap->buckets,
			new_bcount * sizeof(*hashmap->buckets));

	for (i = 0; i < new_bcount; ++i) {
		hashmap->buckets[i] = NULL;
	}

	for (i = 0; i < old_bcount; ++i) {
		iter = old_buckets[i];
		while (iter != NULL) {
			next = iter->next;
			insert(hashmap, iter);
			iter = next;
		}
	}

	free(old_buckets);
}

static void insert(struct hashmap *hashmap, struct hashmap_node *node) {
	unsigned long key_u;
	size_t idx;
	key_u = (unsigned long) hashmap->hash(node->key);
	idx = key_u % hashmap->num_buckets;
	node->next = hashmap->buckets[idx];
	hashmap->buckets[idx] = node;
}

struct hashmap_node **get(struct hashmap *hashmap, void *key) {
	unsigned long key_u;
	size_t idx;
	struct hashmap_node **iter;

	key_u = (unsigned long) hashmap->hash(key);
	idx = key_u % hashmap->num_buckets;
	iter = &hashmap->buckets[idx];
	while (*iter != NULL) {
		if (hashmap->equals((*iter)->key, key)) {
			return iter;
		}
		iter = &(*iter)->next;
	}
	return iter;
}

void hashmap_iter(struct hashmap *hashmap, void *closure,
		void (*callback)(void *closure, void *key, void *value)) {
	size_t i;
	struct hashmap_node *iter;
	for (i = 0; i < hashmap->num_buckets; ++i) {
		iter = hashmap->buckets[i];
		while (iter != NULL) {
			callback(closure, iter->key, iter->value);
			iter = iter->next;
		}
	}
}
