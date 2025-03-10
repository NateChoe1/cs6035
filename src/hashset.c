#include "hashmap.h"
#include "hashset.h"

/* &not_null is not NULL */
static int not_null;

struct hashset_closure {
	void *closure;
	void (*callback)(void *, long);
};
static void iter_callback(void *closure, long key, void *value);

struct hashset *hashset_new(struct arena *arena) {
	return (struct hashset *) hashmap_new(arena);
}

void hashset_put(struct hashset *hashset, long value) {
	hashmap_put((struct hashmap *) hashset, value, &not_null);
}

int hashset_contains(struct hashset *hashset, long value) {
	return hashmap_get((struct hashmap *) hashset, value) != NULL;
}

void hashset_remove(struct hashset *hashset, long value) {
	hashmap_remove((struct hashmap *) hashset, value);
}

void hashset_iter(struct hashset *hashset, void *closure,
		void (*callback)(void *closure, long item)) {
	struct hashset_closure next_closure;

	next_closure.closure = closure;
	next_closure.callback = callback;
	hashmap_iter((struct hashmap *) hashset, &next_closure, iter_callback);
}

static void iter_callback(void *closure_raw, long key, void *value) {
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
