#include "state.h"

static struct state_set_node *new_node(struct arena *arena);

struct state *state_new(struct arena *arena) {
	struct state *ret;
	ret = arena_malloc(arena, sizeof(*ret));
	ret->head = NULL;
	ret->arena = arena;
	return ret;
}

void state_append(struct state *state, void *item) {
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

static struct state_set_node *new_node(struct arena *arena) {
	struct state_set_node *ret;
	ret = arena_malloc(arena, sizeof(*ret));
	ret->n = -1;
	ret->children = hashmap_new(arena);
	return ret;
}
