#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "verify.h"
#include "ordered_map.h"

/*
  Comparison function for an ordered map where we just store
  integers in the key field.
 */
static int cmpfunc(const void *val1, const void *val2) {

  intptr_t i1 = (intptr_t) val1;
  intptr_t i2 = (intptr_t) val2;

  if(i1 < i2)
    return -1;
  else if(i1 > i2)
    return 1;
  else
    return 0;
}


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  const int ntest = 10;

  struct ordered_map *om = ordered_map_new(cmpfunc);
  verify(om);

  /* Try iterating over the empty map */
  verify(ordered_map_iterate(om, NULL)==NULL);

  /* Populate the map with consecutive integer (i, 100*i) key-value pairs */
  for(int i=0; i<ntest; i+=1) {
    intptr_t key = i;
    intptr_t value = 100*i;
    verify(ordered_map_add_item_tail(om, (void *) key, (void *) value) == 0);
  }
  verify(ordered_map_size(om)==ntest);

  /* Iterate over items and check keys and values */
  {
    int j = 0;
    struct ordered_map_item *omi = NULL;
    while((omi = ordered_map_iterate(om, omi))) {
      verify((intptr_t) omi->key == j);
      verify((intptr_t) omi->value == 100*j);
      j += 1;
    }
    verify(j==ntest);
  }

  /* Remove the values in order. This returns the values. */
  for(int i=0; i<ntest; i+=1) {
    intptr_t value = (intptr_t) ordered_map_remove_head(om);
    verify(value==100*i);
  }
  verify(ordered_map_size(om)==0);

  /* Populate the map with consecutive integer (i, 100*i) key-value pairs in reverse order */
  for(int i=0; i<ntest; i+=1) {
    intptr_t key = i;
    intptr_t value = 100*i;
    verify(ordered_map_add_item_head(om, (void *) key, (void *) value) == 0);
  }
  verify(ordered_map_size(om)==ntest);

  /* Iterate over items and check keys and values */
  {
    int j = ntest;
    struct ordered_map_item *omi = NULL;
    while((omi = ordered_map_iterate(om, omi))) {
      j -= 1;
      verify((intptr_t) omi->key == j);
      verify((intptr_t) omi->value == 100*j);
    }
    verify(j==0);
  }

  /* Remove the values in reverse order. */
  for(int i=0; i<ntest; i+=1) {
    intptr_t value = (intptr_t) ordered_map_remove_tail(om);
    verify(value==100*i);
  }
  verify(ordered_map_size(om)==0);

  /* Test removing single elements */
  for(int j=0; j<ntest; j+=1) {

    /* Populate the map */
    for(int i=0; i<ntest; i+=1) {
      intptr_t key = i;
      intptr_t value = 100*i;
      verify(ordered_map_add_item_tail(om, (void *) key, (void *) value) == 0);
    }

    /* Remove an element */
    intptr_t key = (intptr_t) j;
    intptr_t value = (intptr_t) ordered_map_remove_item(om, (void *) key);
    verify(value==j*100);

    /* Check that iteration now skips the removed element */
    struct ordered_map_item *omi = NULL;
    int count = 0;
    while((omi = ordered_map_iterate(om, omi))) {
      verify((intptr_t) omi->value != j*100);
      if(count < j) {
        verify((intptr_t) omi->value == count*100);
      } else {
        verify((intptr_t) omi->value == (count+1)*100);
      }
      count+= 1;
    }
    verify(count==ntest-1);

    /* Clear the map */
    for(int i=0; i<ntest-1; i+=1) {
      ordered_map_remove_head(om);
    }
    verify(ordered_map_size(om)==0);
  }

  ordered_map_free(om);

  return 0;
}
