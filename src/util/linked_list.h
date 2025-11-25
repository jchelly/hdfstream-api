#ifndef __LINKED_LIST_H
#define __LINKED_LIST_H

struct list_item {
  struct list_item *prev;
  struct list_item *next;
  void *data;
};

struct linked_list {
  struct list_item *head;
  struct list_item *tail;
  int count;
};

/* Create a new linked list */
struct linked_list *linked_list_new(void);

/* Add an item at the start of the list */
struct list_item *linked_list_add_item_head(struct linked_list *list, void *data);

/* Add an item at the end of the list */
struct list_item *linked_list_add_item_tail(struct linked_list *list, void *data);

/* Remove first item from the list and return its data pointer so it can be freed */
void *linked_list_remove_head(struct linked_list *list);

/* Remove last item from the list and return its data pointer so it can be freed */
void *linked_list_remove_tail(struct linked_list *list);

/* Make an item the head of the list */
void linked_list_make_item_head(struct linked_list *list, struct list_item *item);

/* Delete an item from anywhere in the list */
void *linked_list_remove_item(struct linked_list *list, struct list_item *item);

/* Free a list */
void linked_list_free(struct linked_list *list);

#endif
