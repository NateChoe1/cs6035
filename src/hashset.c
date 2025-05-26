#include "longmap.h"
#include "hashset.h"

/* &not_null is not NULL */
static int not_null;

struct hashset_closure {
	void *closure;
	void (*callback)(void *, long);
};
static void iter_callback(void *closure, long key, void *value);

struct hashset *hashset_new(struct arena *arena) {
	return (struct hashset *) longmap_new(arena);
}

void hashset_put(struct hashset *hashset, long value) {
	longmap_put((struct longmap *) hashset, value, &not_null);
}

int hashset_contains(struct hashset *hashset, long value) {
	return longmap_get((struct longmap *) hashset, value) != NULL;
}

void hashset_remove(struct hashset *hashset, long value) {
	longmap_remove((struct longmap *) hashset, value);
}

void hashset_iter(struct hashset *hashset, void *closure,
		void (*callback)(void *closure, long item)) {
	struct hashset_closure next_closure;

	next_closure.closure = closure;
	next_closure.callback = callback;
	longmap_iter((struct longmap *) hashset, &next_closure, iter_callback);
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
