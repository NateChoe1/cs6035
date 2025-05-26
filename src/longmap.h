#ifndef LONGMAP_H
#define LONGMAP_H

#include "hashmap.h"

struct longmap;

struct longmap *longmap_new(struct arena *arena);
void longmap_put(struct longmap *longmap, long key, void *value);
void *longmap_get(struct longmap *longmap, long key);
void longmap_remove(struct longmap *longmap, long key);
void longmap_iter(struct longmap *longmap, void *closure,
		void (*callback)(void *closure, long key, void *value));

#endif
