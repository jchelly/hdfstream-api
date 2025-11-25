#include <stdlib.h>
#include <assert.h>

#include "avltree.h"
#include "linked_list.h"
#include "ordered_map.h"


struct ordered_map *ordered_map_new(int (*cmpfunc) (const void *val1, const void *val2)) {

  struct ordered_map *om = malloc(sizeof(struct ordered_map));
  om->cmpfunc = cmpfunc;
  om->tree = avltree_new(cmpfunc);
  om->list = linked_list_new();
  return om;
}


int ordered_map_add_item_tail(struct ordered_map *om, void *key, void *value) {

  /* Return failure if the key already exists */
  if(avltree_lookup(om->tree, key))return -1;

  /* Create the new ordered map item */
  struct ordered_map_item *omi = malloc(sizeof(struct ordered_map_item));
  omi->key = key;
  omi->value = value;

  /* Add the ordered map item to the linked list */
  struct list_item *li = linked_list_add_item_tail(om->list, omi);
  omi->list_item = li;

  /* Add the ordered map item to the tree */
  int err = avltree_add_node(om->tree, key, omi);
  assert(err==0);

  /* Success! */
  return err;
}


int ordered_map_add_item_head(struct ordered_map *om, void *key, void *value) {

  /* Return failure if the key already exists */
  if(avltree_lookup(om->tree, key))return -1;

  /* Create the new ordered map item */
  struct ordered_map_item *omi = malloc(sizeof(struct ordered_map_item));
  omi->key = key;
  omi->value = value;

  /* Add the ordered map item to the linked list */
  struct list_item *li = linked_list_add_item_head(om->list, omi);
  omi->list_item = li;

  /* Add the ordered map item to the tree */
  int err = avltree_add_node(om->tree, key, omi);
  assert(err==0);

  /* Success! */
  return err;
}


void *ordered_map_remove_item(struct ordered_map *om, const void *key) {

  /* Return failure if the key does not exist */
  if(!avltree_lookup(om->tree, key))return NULL;

  /* Locate the ordered map item in the tree */
  struct ordered_map_item *omi = avltree_lookup(om->tree, key);

  /* Find the linked list item */
  struct list_item *li = omi->list_item;

  /* Remove the item from the tree */
  avltree_delete_node(om->tree, key);

  /* Remove the item from the linked list */
  linked_list_remove_item(om->list, li);

  /* Free the map item and return the associated value */
  char *value = omi->value;
  free(omi);
  return (void *) value;
}


void *ordered_map_remove_head(struct ordered_map *om) {

  /* Return failure if there are no entries */
  if(om->list->count==0)return NULL;

  /* Get the list item */
  struct list_item *li = om->list->head;

  /* Find the key and value corresponding to the item */
  struct ordered_map_item *omi = li->data;
  assert(omi->list_item==li);
  char *key = omi->key;
  char *value = omi->value;

  /* Remove the item from the tree */
  avltree_delete_node(om->tree, key);

  /* Remove the item from the linked list */
  linked_list_remove_item(om->list, li);

  /* Free the map item and return the associated value */
  free(omi);
  return (void *) value;
}


void *ordered_map_remove_tail(struct ordered_map *om) {

  /* Return failure if there are no entries */
  if(om->list->count==0)return NULL;

  /* Get the list item */
  struct list_item *li = om->list->tail;

  /* Find the key and value corresponding to the item */
  struct ordered_map_item *omi = li->data;
  assert(omi->list_item==li);
  char *key = omi->key;
  char *value = omi->value;

  /* Remove the item from the tree */
  avltree_delete_node(om->tree, key);

  /* Remove the item from the linked list */
  linked_list_remove_item(om->list, li);

  /* Free the map item and return the associated value */
  free(omi);
  return (void *) value;
}


int ordered_map_size(struct ordered_map *om) {
  return om->list->count;
}


void *ordered_map_lookup(struct ordered_map *om, const void *key) {

  struct ordered_map_item *omi = avltree_lookup(om->tree, key);
  if(omi)
    return omi->value;
  else
    return NULL;
}


int ordered_map_make_item_head(struct ordered_map *om, const void *key) {

  struct ordered_map_item *omi = avltree_lookup(om->tree, key);
  if(!omi) return -1;

  linked_list_make_item_head(om->list, omi->list_item);
  return 0;
}


void ordered_map_free(struct ordered_map *om) {

  /*
    Free any elements.
    Note that if element values are pointers to objects which need to be freed
    you should ensure the map is empty before calling this function.
  */
  while(ordered_map_size(om) > 0) {
    ordered_map_remove_tail(om);
  }

  linked_list_free(om->list);
  avltree_free(om->tree);
  free(om);
}


/*
  Iterate over items in the ordered map.

  The input pointer should be null to start iterating. Returns null when there
  are no more items.
*/
struct ordered_map_item *ordered_map_iterate(struct ordered_map *om, struct ordered_map_item *omi) {

  if(!omi) {
    /* Null input. Need to find the first item, if there is one. */
    struct list_item *li = om->list->head;
    if(li) {
      /* Return a pointer to the first item */
      return li->data;
    } else {
      /* Map contains no items */
      return NULL;
    }
  } else {
    /* Input is an item in the map. Find the next one.  */
    struct list_item *li = omi->list_item->next;
    if(li) {
      /* Return a pointer to the next map item */
      return li->data;
    } else {
      /* Reached the end of the linked list */
      return NULL;
    }
  }
}

