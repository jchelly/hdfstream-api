#ifndef SHARED_DATA_H_
#define SHARED_DATA_H_

#include <stdlib.h>
#include <semaphore.h>

#define ALIGNMENT 1024
#define NR_BUFFERS 3

#define BUFFER_STATUS_OK      0
#define BUFFER_STATUS_END     1
#define BUFFER_STATUS_ERROR   2
#define BUFFER_STATUS_CANCEL  3

/*
  Information about a buffer in shared memory

  Here we store an offset to the data rather than a pointer because the
  shared region is mapped at a different starting address on each process.
*/
struct buffer_data {
  size_t offset;          /* Start of buffer relative to the start of shared memory, in bytes */
  size_t used;            /* Number of bytes written to the buffer */
  sem_t ready_for_write;  /* Semaphore to indicate if buffer is available to overwrite */
  sem_t ready_for_read;   /* Semaphore to indicate if buffer contains data ready to be read out */
  int status;             /* Status code to indicate success or failure */
};

/*
  Information stored in shared memory for each reader process

  Shared memory contains an instance of struct shared_reader_data then
  NR_BUFFERS buffers of size buffer_size.
*/
struct shared_reader_data {

  int    status;            /* Stream status code */
  size_t total_size;        /* Full size of the shared memory region */
  size_t buffer_size;       /* Size of each buffer */
  struct buffer_data buffer[NR_BUFFERS];

  /* Diagnostic info from the process */
  int nr_file_cache_hits;
  int nr_file_cache_misses;
};

/* Function to compute total amount of shared memory needed */
size_t shared_data_size(const size_t buffer_size);

/* Function to set up array of structs in shared memory */
void shared_data_populate(void *ptr, const size_t buffer_size);

/* Function to free semaphores etc in shared memory */
void shared_data_free(void *ptr);

/* Set semaphores that control buffer access to initial state for a new request */
void shared_data_init_semaphores(struct shared_reader_data *srd);

#endif
