#include "sb.h"

struct sb *sb_new(struct arena *arena) {
	struct sb *ret;
	ret = arena_malloc(arena, sizeof(*ret));
	ret->arena = arena;
	ret->len = 0;
	ret->alloc = 32;
	ret->data = arena_malloc(arena, ret->alloc);
	return ret;
}

void sb_append(struct sb *sb, char c) {
	if (sb->len >= sb->alloc) {
		sb->alloc *= 2;
		sb->data = arena_realloc(sb->data, sb->alloc);
	}
	sb->data[sb->len++] = c;
}

char *sb_read(struct sb *sb) {
	sb_append(sb, '\0');
	--sb->len;
	return sb->data;
}
