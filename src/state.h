#ifndef STATE_H
#define STATE_H

#include "arena.h"
#include "hashmap.h"

/* a state is a set of items. this notion of a state comes up a lot in parsing;
 * for example to convert an nfa to a dfa each state in the dfa becomes a set of
 * states in the nfa. also, the closure() function in lr parsers corresponds to
 * this idea as well. */

/* linked list of item ids, sorted in ascending order
 *
 * we maintain this sorting with a basic insertion sort. this means that
 * creating a new state takes O(n^2) time where n is the number of items in the
 * state. i think this is fine since states are usually pretty small.
 * */
struct state_item {
	long value;
	struct state_item *next;
};

struct state {
	struct state_item *head;
	struct arena *arena;
};

/* a state is a set of items, which are just void * pointers.
 *
 * a state is internally represented as a linked list of pointers which is
 * guaranteed to be sorted in ascending order.
 *
 * this linked list of pointers can be treated as a string, and we can use a
 * trie to store a set of linked lists of pointers. */
struct state_map_node {
	/* the data associated with this state */
	long n;

	/* maps items to struct state_map_node * pointers */
	struct hashmap *children;
};

struct state_map {
	struct state_map_node *root;
	struct arena *arena;
};

struct state_list {
	struct arena *arena;
	struct state **states;
	size_t len;
	size_t alloc;
};

/* an ordered state is a dynamic array of items */
struct state_ordered {
	struct arena *arena;
	long *items;
	size_t size;
	size_t alloc;
};

struct state *state_new(struct arena *arena);
void state_append(struct state *state, long item);

/* equivalent to base += additions */
void state_join(struct state *base, struct state *additions);

struct state_map *state_map_new(struct arena *arena);
void state_map_put(struct state_map *set, struct state *state, long n);

/* return -1 if state isn't in set. */
long state_map_get(struct state_map *set, struct state *state);

struct state_list *state_list_new(struct arena *arena);
void state_list_add(struct state_list *list, struct state *state);

struct state_ordered *state_ordered_new(struct arena *arena);
void state_ordered_put(struct state_ordered *state, long value);

#endif
