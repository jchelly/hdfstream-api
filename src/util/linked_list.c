#include <stdlib.h>
#include <assert.h>

#include "verify.h"
#include "linked_list.h"

#ifndef NDEBUG
static void linked_list_check(struct linked_list *list) {

  struct list_item *this_item;
  struct list_item *prev_item;
  int nr_items = 0;

  this_item = list->head;
  prev_item = NULL;
  while(this_item) {
    if(this_item==list->head) {
      verify(this_item->prev==NULL);
    } else {
      verify(prev_item->next == this_item);
      verify(this_item->prev == prev_item);
    }
    prev_item = this_item;
    this_item=this_item->next;
    nr_items += 1;
  }
  verify(nr_items==list->count);
}
#endif

/* Create a new linked list */
struct linked_list *linked_list_new(void) {

  struct linked_list *list = malloc(sizeof(struct linked_list));
  list->head = NULL;
  list->tail = NULL;
  list->count = 0;

#ifndef NDEBUG
  linked_list_check(list);
#endif

  return list;
}


/* Add an item at the head of the list */
struct list_item *linked_list_add_item_head(struct linked_list *list, void *data) {

  struct list_item *new_item = malloc(sizeof(struct list_item));
  new_item->data = data;
  new_item->prev = NULL;
  new_item->next = list->head;
  if(list->head)
    list->head->prev = new_item;
  else
    list->tail=new_item;
  list->head = new_item;
  list->count += 1;

#ifndef NDEBUG
  linked_list_check(list);
#endif

  return new_item;
}


/* Add an item at the tail of the list */
struct list_item *linked_list_add_item_tail(struct linked_list *list, void *data) {

  struct list_item *new_item = malloc(sizeof(struct list_item));
  new_item->data = data;
  new_item->next = NULL;
  new_item->prev = NULL;
  if(list->count==0) {
    /* This is the first item, so it is the head AND tail */
    list->head = new_item;
    list->tail = new_item;
  } else {
    /* List already contains item(s) */
    new_item->prev = list->tail;
    list->tail->next = new_item;
    list->tail = new_item;
  }
  list->count += 1;

#ifndef NDEBUG
  linked_list_check(list);
#endif

  return new_item;
}


/* Make an item the head of the list */
void linked_list_make_item_head(struct linked_list *list, struct list_item *item) {

  /* If it's already the head, we have nothing to do */
  if(item==list->head)return;

  struct list_item *old_head = list->head;
  struct list_item *old_prev = item->prev;
  struct list_item *old_next = item->next;

  /* If this item is the tail, set previous item as new tail */
  if(list->tail == item) {
    assert(old_prev);
    list->tail = old_prev;
  }

  /* Make this item the new head */
  list->head = item;
  item->prev = NULL;
  item->next = old_head;
  if(old_head)old_head->prev=item;

  /* Link previous and next nodes adjacent to where we removed the item */
  if(old_prev)
    old_prev->next = old_next;
  if(old_next)
    old_next->prev = old_prev;

#ifndef NDEBUG
  linked_list_check(list);
#endif
}


/* Remove an item from the list and return its data pointer */
void *linked_list_remove_tail(struct linked_list *list) {

  assert(list->count > 0);
  struct list_item *item = list->tail;
  void *data = item->data;

  if(list->count == 1) {
    /* We're removing the only item */
    list->head = NULL;
    list->tail = NULL;
  }else{
    /* There's one or more other items left */
    list->tail = item->prev;
    item->prev->next=NULL;
  }
  free(item);
  list->count -= 1;

#ifndef NDEBUG
  linked_list_check(list);
#endif

  return data;
}


/* Remove an item from the list and return its data pointer */
void *linked_list_remove_head(struct linked_list *list) {

  assert(list->count > 0);
  struct list_item *item = list->head;
  void *data = item->data;

  if(list->count == 1) {
    /* We're removing the only item */
    list->head = NULL;
    list->tail = NULL;
  }else{
    /* There's one or more other items left */
    list->head = item->next;
    item->next->prev = NULL;
  }
  free(item);
  list->count -= 1;

#ifndef NDEBUG
  linked_list_check(list);
#endif

  return data;
}


/* Delete an item from anywhere in the list */
void *linked_list_remove_item(struct linked_list *list, struct list_item *item) {

  linked_list_make_item_head(list, item);
  return linked_list_remove_head(list);
}


/* Free an empty list */
void linked_list_free(struct linked_list *list) {

#ifndef NDEBUG
  linked_list_check(list);
#endif
  assert(list->count==0);
  free(list);
}


