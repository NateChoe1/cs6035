#include "state.h"

static struct state_set_node *new_node(struct arena *arena);

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

	new_item->value = item;
	new_item->next = *ptr;
	*ptr = new_item;
}

struct state_set *state_set_new(struct arena *arena) {
	struct state_set *ret;

	ret = arena_malloc(arena, sizeof(*ret));
	ret->root = new_node(arena);
	ret->arena = arena;
	return ret;
}

void state_set_put(struct state_set *set, struct state *state, int n) {
	struct state_set_node *trie_iter, *trie_tmp;
	struct state_item *state_iter;
	trie_iter = set->root;
	state_iter = state->head;
	while (state_iter != NULL) {
		trie_tmp = hashmap_get(trie_iter->children, state_iter->value);
		if (trie_tmp == NULL) {
			trie_tmp = new_node(set->arena);
			hashmap_put(trie_iter->children,
					state_iter->value, (void *) trie_tmp);
		}

		trie_iter = trie_tmp;
		state_iter = state_iter->next;
	}

	trie_iter->n = n;
}

long state_set_get(struct state_set *set, struct state *state) {
	struct state_set_node *trie_iter;
	struct state_item *state_iter;

	trie_iter = set->root;
	state_iter = state->head;

	while (state_iter != NULL) {
		trie_iter = hashmap_get(trie_iter->children, state_iter->value);
		if (trie_iter == NULL) {
			return -1;
		}
		state_iter = state_iter->next;
	}
	return trie_iter->n;
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

static struct state_set_node *new_node(struct arena *arena) {
	struct state_set_node *ret;
	ret = arena_malloc(arena, sizeof(*ret));
	ret->n = -1;
	ret->children = hashmap_new(arena);
	return ret;
}
