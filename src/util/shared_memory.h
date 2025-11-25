#ifndef SHARED_MEMORY_H_
#define SHARED_MEMORY_H_

#include <stddef.h>

/*
  Functions for managing shared memory. Intended use:

  Master process calls shared_memory_new()
  Other processes call shared_memory_map() to access the region
  Master calls shared_memory_unlink()

  When the region is no longer needed, all processes call shared_memory_unmap().
*/

struct shared_memory {
  char *name;
  size_t size;
  int fd;
  char *data;
};

/* Create and map a new shared memory region. Returns NULL on failure. */
struct shared_memory *shared_memory_new(const char *name, const size_t size);

/* Map an existing shared memory region. Returns NULL on failure. */
struct shared_memory *shared_memory_map(const char *name, const size_t size);

/* Unlink a shared memory region */
int shared_memory_unlink(struct shared_memory *sm);

/* Unmap a shared memory region */
void shared_memory_unmap(struct shared_memory *sm);

#endif
