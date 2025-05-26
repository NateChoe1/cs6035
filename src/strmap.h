#ifndef STRMAP_H
#define STRMAP_H

#include "arena.h"

struct strmap;

struct strmap *strmap_new(struct arena *arena);
void strmap_put(struct strmap *strmap, const char *key, const void *value);
void *strmap_get(struct strmap *strmap, const char *key);
void strmap_remove(struct strmap *strmap, const char *key);
void strmap_iter(struct strmap *strmap, void *closure,
		void (*callback)(void *closure, const char *key, void *value));

#endif
