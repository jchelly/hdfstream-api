#define _POSIX_C_SOURCE 200809L
#include "shared_memory.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>


/* Create a new shared memory region. Returns NULL on failure. */
struct shared_memory *shared_memory_new(const char *name, const size_t size) {

  /* Create shared memory file in writable mode with u+rw permissions */
  int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
  if(fd < 0)return NULL; /* Failed to create the file */

  /* Set the size of the file */
  if(ftruncate(fd, (off_t) size) < 0) {
    shm_unlink(name);
    close(fd);
    return NULL;
  }

  /* Memory map the new file */
  char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(data == MAP_FAILED) {
    shm_unlink(name);
    close(fd);
    return NULL;
  }

  /* If that all worked, return a shared_memory struct */
  struct shared_memory *sm = malloc(sizeof(struct shared_memory));
  sm->size = size;
  sm->data = data;
  sm->fd = fd;
  sm->name = malloc(strlen(name)+1);
  strcpy(sm->name, name);

  return sm;
}

/* Map an existing shared memory region. Returns NULL on failure. */
struct shared_memory *shared_memory_map(const char *name, const size_t size) {

  /* Open an existing shared memory file */
  int fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
  if(fd < 0)return NULL; /* Failed to open the file */

  /* Memory map the file we opened */
  char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(data == MAP_FAILED) {
    close(fd);
    return NULL;
  }

  /* If that all worked, return a shared_memory struct */
  struct shared_memory *sm = malloc(sizeof(struct shared_memory));
  sm->size = size;
  sm->data = data;
  sm->fd = fd;
  sm->name = malloc(strlen(name)+1);
  strcpy(sm->name, name);

  return sm;
}

/* Unlink a shared memory region. Returns non zero on failure. */
int shared_memory_unlink(struct shared_memory *sm) {
  return shm_unlink(sm->name);
}

/* Unmap and close a shared memory region. Also frees the struct. */
void shared_memory_unmap(struct shared_memory *sm) {
  munmap(sm->data, sm->size);
  close(sm->fd);
  free(sm->name);
  free(sm);
}
