#ifndef SB_H
#define SB_H

/* string builder */

#include "arena.h"

struct sb {
	struct arena *arena;
	char *data;
	size_t len;
	size_t alloc;
};

struct sb *sb_new(struct arena *arena);
void sb_append(struct sb *sb, char c);
char *sb_read(struct sb *sb);

#endif
