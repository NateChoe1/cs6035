#include <string.h>

#include "strmap.h"
#include "hashmap.h"

static long hash(const void *ptr);
static int equals(const void *s1, const void *s2);

struct strmap_closure {
	void *closure;
	void (*callback)(void *closure, const char *key, void *value);
};

static void s_callback(void *closure, const void *key, void *value);

struct strmap *strmap_new(struct arena *arena) {
	return (struct strmap *) hashmap_new(arena, hash, equals);
}

void strmap_put(struct strmap *strmap, const char *key, const void *value) {
	hashmap_put((struct hashmap *) strmap, key, value);
}

void *strmap_get(struct strmap *strmap, const char *key) {
	return hashmap_get((struct hashmap *) strmap, key);
}

void strmap_remove(struct strmap *strmap, const char *key) {
	hashmap_remove((struct hashmap *) strmap, key);
}

void strmap_iter(struct strmap *strmap, void *closure,
		void (*callback)(void *closure, const char *key, void *value)) {
	struct strmap_closure encapsulated;
	encapsulated.closure = closure;
	encapsulated.callback = callback;
	hashmap_iter((struct hashmap *) strmap, &encapsulated, s_callback);
}

static void s_callback(void *closure, const void *key, void *value) {
	struct strmap_closure *encapsulated;
	encapsulated = closure;
	encapsulated->callback(encapsulated->closure, key, value);
}

static long hash(const void *ptr) {
	const char *str = ptr;
	int i;
	long ret;

	ret = 0;
	for (i = 0; str[i]; ++i) {
		ret *= 31;
		ret += str[i];
	}

	return ret;
}

static int equals(const void *s1, const void *s2) {
	return strcmp((const char *) s1, (const char *) s2) == 0;
}
