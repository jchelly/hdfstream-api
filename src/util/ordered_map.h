#ifndef ORDERED_MAP_H
#define ORDERED_MAP_H

#include "avltree.h"
#include "linked_list.h"

/*
  Data structure to maintain an ordered list of (key, value) pairs.
  Both keys and values are stored as void pointers.

  Note that we can't store NULL keys or values because NULL is used
  to indicate failure.
*/

struct ordered_map {
  int (*cmpfunc) (const void *val1, const void *val2);
  AVLTree *tree;
  struct linked_list *list;
};

struct ordered_map_item {
  void *key;
  void *value;
  struct list_item *list_item;
};

/* Create a new ordered map */
struct ordered_map *ordered_map_new(int (*cmpfunc) (const void *val1, const void *val2));

/*
  Add an entry to the map in the last position. Fail if entry exists.
  Returns 0 on success, non-zero on failure.
*/
int ordered_map_add_item_tail(struct ordered_map *om, void *key, void *value);

/* As above, but add the item to the head of the list */
int ordered_map_add_item_head(struct ordered_map *om, void *key, void *value);

/* Remove a key from the map. Returns the associated data pointer so it can be freed. */
void *ordered_map_remove_item(struct ordered_map *om, const void *key);

/* Remove the first item */
void *ordered_map_remove_head(struct ordered_map *om);

/* Remove the last item */
void *ordered_map_remove_tail(struct ordered_map *om);

/* Return the number of entries */
int ordered_map_size(struct ordered_map *om);

/* Return the value associated with a key, or NULL if it doesn't exist */
void *ordered_map_lookup(struct ordered_map *om, const void *key);

/* Move a key to the first or last position, returns 0 on success, non-zero on failure */
int ordered_map_make_item_head(struct ordered_map *om, const void *key);

/* Free the map. Map should be empty if values are pointers which need to be freed. */
void ordered_map_free(struct ordered_map *om);

/* Iterate over items in the map */
struct ordered_map_item *ordered_map_iterate(struct ordered_map *om, struct ordered_map_item *omi);

#endif
