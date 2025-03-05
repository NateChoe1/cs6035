#include "hashmap.h"
#include "hashset.h"

/* &not_null is not NULL */
static int not_null;

struct hashset_closure {
	void *closure;
	void (*callback)(void *, void *);
};
static void iter_callback(void *closure, void *key, void *value);

struct hashset *hashset_new(struct arena *arena) {
	return (struct hashset *) hashmap_new(arena);
}

void hashset_put(struct hashset *hashset, void *ptr) {
	hashmap_put((struct hashmap *) hashset, ptr, &not_null);
}

int hashset_contains(struct hashset *hashset, void *ptr) {
	return hashmap_get((struct hashmap *) hashset, ptr) != NULL;
}

void hashset_remove(struct hashset *hashset, void *ptr) {
	hashmap_remove((struct hashmap *) hashset, ptr);
}

void hashset_free(struct hashset *hashset) {
	hashmap_free((struct hashmap *) hashset);
}

void hashset_iter(struct hashset *hashset, void *closure,
		void (*callback)(void *closure, void *item)) {
	struct hashset_closure next_closure;

	next_closure.closure = closure;
	next_closure.callback = callback;
	hashmap_iter((struct hashmap *) hashset, &next_closure, iter_callback);
}

static void iter_callback(void *closure_raw, void *key, void *value) {
	struct hashset_closure *closure;

	if (value == NULL) {
		return;
	}
	closure = (struct hashset_closure *) closure_raw;
	closure->callback(closure->closure, key);
}

size_t hashset_size(struct hashset *hashset) {
	return ((struct hashmap *) hashset)->num_items;
}
