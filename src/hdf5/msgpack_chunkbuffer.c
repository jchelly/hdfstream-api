#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <semaphore.h>
#include <errno.h>

#include "worker_process.h"
#include "msgpack_chunkbuffer.h"
#include "commands.h"

static void semaphore_wait(sem_t *s) {
  int err;
  do {
    err = sem_wait(s);
  } while(err == EINTR);
}

/*
  Allocate a new msgpack chunk buffer
*/
int msgpack_chunkbuffer_init(struct msgpack_chunkbuffer *cb, size_t buffer_size,
                             void *shared_data, struct shared_reader_data *srd) {

  assert(buffer_size > 0);
  assert(buffer_size <= srd->buffer_size); /* Requested buffer size may not exceed allocated shared memory */
  cb->shared_data = shared_data;
  cb->srd = srd;
  cb->buffer_nr = -1;
  cb->buffer_size = buffer_size;
  return 0;
}

/*
  Flush the buffer - i.e. allow the reader process to read it.
*/
int msgpack_chunkbuffer_flush(struct msgpack_chunkbuffer *cb) {

  /* If we've been asked to abort, return unsuccessfully */
  if(cb->srd->status != BUFFER_STATUS_OK)return -1;

  /* If we didn't claim a buffer yet there's nothing to do */
  if( cb->buffer_nr < 0)return 0;

  /* Get a pointer to the current buffer */
  struct buffer_data *buffer = cb->srd->buffer + cb->buffer_nr;

  /* If the buffer is empty there's nothing to do */
  if(buffer->used==0)return 0;

  /* Mark this buffer as readable */
  sem_post(&(buffer->ready_for_read));

  /* Advance to the next buffer */
  cb->buffer_nr = (cb->buffer_nr + 1) % NR_BUFFERS;
  buffer = cb->srd->buffer + cb->buffer_nr;

  /* Wait for the new buffer to be available for writing */
  semaphore_wait(&(buffer->ready_for_write));

  return 0;
}

/*
  Free a chunk buffer. Ends the data stream and appends a return code.
  If return code is non-zero we discard any buffered data.
*/
int msgpack_chunkbuffer_destroy(struct msgpack_chunkbuffer *cb, int return_code) {

  /* Flush the buffer */
  msgpack_chunkbuffer_flush(cb);

  /* Might need to claim a buffer to write to if no data was written */
  if(cb->buffer_nr < 0) {
    cb->buffer_nr = 0;
    struct buffer_data *buffer = cb->srd->buffer + cb->buffer_nr;
    semaphore_wait(&(buffer->ready_for_write));
    buffer->used = 0;
  }
  struct buffer_data *buffer = cb->srd->buffer + cb->buffer_nr;

  /* Send a zero sized block with the return code */
  buffer->used = 0;
  buffer->status = return_code;

  /* Mark the buffer as readable */
  sem_post(&(buffer->ready_for_read));

  /* Make the chunkbuffer unusable */
  cb->buffer_size = 0;
  cb->shared_data = NULL;

  return 0;
}

/*
  Write bytes to the chunk buffer

  data: pointer to a struct msgpack_chunkbuffer
  buf: buffer with data to write
  len: size of buffer to write

  Returns 0 on success, -1 otherwise

*/
int msgpack_chunkbuffer_write(void *data, const char* buf, size_t len)
{
  struct msgpack_chunkbuffer *cb = (struct msgpack_chunkbuffer *) data;

  /* On the first call we need to claim a buffer to write to */
  if(cb->buffer_nr < 0) {
    cb->buffer_nr = 0;
    struct buffer_data *buffer = cb->srd->buffer + cb->buffer_nr;
    semaphore_wait(&(buffer->ready_for_write));
    buffer->used = 0;
  }

  const char *ptr = buf;
  size_t left_to_write = len;
  while(left_to_write > 0) {

    /* Get a pointer to the buffer. Note that msgpack_chunkbuffer_flush()
       invalidates this pointer by switching the active buffer. */
    struct buffer_data *buffer = cb->srd->buffer + cb->buffer_nr;

    /* Compute how many bytes we can add to the buffer */
    size_t bytes_free = cb->buffer_size - buffer->used;
    size_t nr_to_copy = left_to_write;
    if(nr_to_copy > bytes_free)nr_to_copy = bytes_free;

    /* Copy the data to the buffer */
    char *dest = cb->shared_data + buffer->offset + buffer->used;
    memcpy(dest, ptr, nr_to_copy);
    buffer->used += nr_to_copy;
    ptr += nr_to_copy;
    left_to_write -= nr_to_copy;

    /* If the buffer is now at least half full, flush it to the file. This
       ensures we always have enough free space to H5Dread() half a buffer
       of slice data directly into shared memory without copying it. */
    if(buffer->used >= cb->buffer_size/2) {
      if(msgpack_chunkbuffer_flush(cb) < 0)return -1;
    }
  }
  return 0;
}
