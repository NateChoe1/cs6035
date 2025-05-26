#include "longmap.h"

static void callback(void *closure, const void *key, void *value);
static long hash(const void *ptr);
static int eq(const void *ptr1, const void *ptr2);

struct closure {
	void (*callback)(void *closure, long key, void *value);
	void *closure;
};

struct longmap *longmap_new(struct arena *arena) {
	return (struct longmap *) hashmap_new(arena, hash, eq);
}

void longmap_put(struct longmap *longmap, long key, void *value) {
	hashmap_put((struct hashmap *) longmap, (void *) key, value);
}

void *longmap_get(struct longmap *longmap, long key) {
	return hashmap_get((struct hashmap *) longmap, (void *) key);
}

void longmap_remove(struct longmap *longmap, long key) {
	hashmap_remove((struct hashmap *) longmap, (void *) key);
}

void longmap_iter(struct longmap *longmap, void *closure,
		void (*callback_f)(void *closure, long key, void *value)) {
	struct closure encapsulated;
	encapsulated.callback = callback_f;
	encapsulated.closure = closure;
	hashmap_iter((struct hashmap *) longmap, &encapsulated, callback);
}

static void callback(void *closure, const void *key, void *value) {
	struct closure *encapsulated;
	encapsulated = closure;
	encapsulated->callback(encapsulated->closure, (long) key, value);
}

static long hash(const void *ptr) {
	return (long) ptr;
}

static int eq(const void *ptr1, const void *ptr2) {
	return ptr1 == ptr2;
}
