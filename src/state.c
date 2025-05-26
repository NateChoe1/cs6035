#include "state.h"

static struct state_map_node *new_node(struct arena *arena);
static struct state_map_node *find(struct state_map *map, struct state *state,
		int create);
static struct state_map_node *step(struct arena *arena,
		struct state_map_node *node, long item, int create);

struct state *state_new(struct arena *arena) {
	struct state *ret;
	ret = arena_malloc(arena, sizeof(*ret));
	ret->head = NULL;
	ret->arena = arena;
	return ret;
}

void state_append(struct state *state, long item) {
	struct state_item **ptr, *new_item;

	new_item = arena_malloc(state->arena, sizeof(*new_item));

	ptr = &state->head;
	while (*ptr != NULL && (*ptr)->value < item) {
		ptr = &(*ptr)->next;
	}

	/* states can't contain duplicate items */
	if (*ptr != NULL && (*ptr)->value == item) {
		return;
	}

	new_item->value = item;
	new_item->next = *ptr;
	*ptr = new_item;
}

void state_join(struct state *base, struct state *additions) {
	struct state_item *i, **p, *n;
	i = additions->head;
	p = &base->head;
	while (i != NULL) {
		while (*p != NULL && (*p)->value < i->value) {
			p = &(*p)->next;
		}
		if (*p != NULL && (*p)->value == i->value) {
			goto skip;
		}
		n = arena_malloc(base->arena, sizeof(*n));
		n->value = i->value;
		n->next = (*p != NULL) ? (*p)->next : NULL;
		*p = n;
skip:
		i = i->next;
	}
}

struct state_map *state_map_new(struct arena *arena) {
	struct state_map *ret;

	ret = arena_malloc(arena, sizeof(*ret));
	ret->root = new_node(arena);
	ret->arena = arena;
	return ret;
}

void state_map_put(struct state_map *set, struct state *state, long n) {
	find(set, state, 1)->n = n;
}

long state_map_get(struct state_map *set, struct state *state) {
	struct state_map_node *node;

	node = find(set, state, 0);
	if (node == NULL) {
		return -1;
	}
	return node->n;
}

struct state_list *state_list_new(struct arena *arena) {
	struct state_list *ret;
	ret = arena_malloc(arena, sizeof(*ret));
	ret->arena = arena;
	ret->len = 0;
	ret->alloc = 32;
	ret->states = arena_malloc(arena, ret->alloc * sizeof(*ret->states));
	return ret;
}

void state_list_add(struct state_list *list, struct state *state) {
	if (list->len >= list->alloc) {
		list->alloc *= 2;
		list->states = arena_realloc(list->states,
				list->alloc * sizeof(*list->states));
	}
	list->states[list->len++] = state;
}

struct state_ordered *state_ordered_new(struct arena *arena) {
	struct state_ordered *ret;
	ret = arena_malloc(arena, sizeof(*ret));
	ret->arena = arena;
	ret->size = 0;
	ret->alloc = 32;
	ret->items = arena_malloc(arena, ret->alloc * sizeof(*ret->items));
	return ret;
}

void state_ordered_put(struct state_ordered *state, long value) {
	if (state->size >= state->alloc) {
		state->alloc *= 2;
		state->items = arena_realloc(state->items,
				state->alloc * sizeof(*state->items));
	}
	state->items[state->size++] = value;
}

static struct state_map_node *new_node(struct arena *arena) {
	struct state_map_node *ret;
	ret = arena_malloc(arena, sizeof(*ret));
	ret->n = -1;
	ret->children = longmap_new(arena);
	return ret;
}

static struct state_map_node *find(struct state_map *map, struct state *state,
		int create) {
	struct state_map_node *node;
	struct state_item *iter;

	node = map->root;
	iter = state->head;
	while (iter != NULL) {
		node = step(map->arena, node, iter->value, create);
		if (node == NULL) {
			return NULL;
		}
		iter = iter->next;
	}
	return node;
}

static struct state_map_node *step(struct arena *arena,
		struct state_map_node *node, long item, int create) {
	struct state_map_node *ret;
	ret = longmap_get(node->children, item);
	if (ret == NULL) {
		if (!create) {
			return NULL;
		}
		ret = new_node(arena);
		longmap_put(node->children, item, (void *) ret);
	}
	return ret;
}
