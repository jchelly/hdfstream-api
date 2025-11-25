#include "shared_data.h"


static size_t header_size(void) {

  size_t size = sizeof(struct shared_reader_data);
  if(size % ALIGNMENT != 0) {
    size -= (size % ALIGNMENT);
    size += ALIGNMENT;
  }
  return size;
}


size_t shared_data_size(const size_t buffer_size) {

  /* Calculate size of header */
  size_t shared_size = header_size();

  /* Add on the required buffer sizes */
  shared_size += NR_BUFFERS*buffer_size;

  return shared_size;
}


void shared_data_populate(void *ptr, const size_t buffer_size) {

  size_t buffer_offset = header_size();
  size_t total_size = shared_data_size(buffer_size);

  struct shared_reader_data *srd = (struct shared_reader_data *) ptr;

  srd->total_size = total_size;
  srd->buffer_size = buffer_size;
  srd->nr_file_cache_hits = 0;
  srd->nr_file_cache_misses = 0;
  for(int j=0; j<NR_BUFFERS; j+=1) {
    srd->buffer[j].offset = buffer_offset + j*buffer_size;
    srd->buffer[j].used = 0;
    srd->buffer[j].status = 0;
    sem_init(&(srd->buffer[j].ready_for_write), /* pshared = */ 1, /* value = */ 0);
    sem_init(&(srd->buffer[j].ready_for_read), /* pshared = */ 1, /* value = */ 0);
  }
}

void shared_data_free(void *ptr) {

  struct shared_reader_data *srd = (struct shared_reader_data *) ptr;
  for(int j=0; j<NR_BUFFERS; j+=1) {
    sem_destroy(&(srd->buffer[j].ready_for_write));
    sem_destroy(&(srd->buffer[j].ready_for_read));
  }
}

/*
  Set initial semaphore state before processing a request:
  All buffers should be flagged as writable but not readable.
*/
void shared_data_init_semaphores(struct shared_reader_data *srd) {

  int value;

  /* Buffers are initially not available for reading */
  for(int i=0; i<NR_BUFFERS; i+=1) {
    sem_t *s = &(srd->buffer[i].ready_for_read);
    sem_getvalue(s, &value);
    while(value > 0) {
      sem_wait(s);
      sem_getvalue(s, &value);
    };
  }

  /* Buffers initially are available for writing */
  for(int i=0; i<NR_BUFFERS; i+=1) {
    sem_t *s = &(srd->buffer[i].ready_for_write);
    sem_getvalue(s, &value);
    while(value < 1) {
      sem_post(s);
      sem_getvalue(s, &value);
    };
  }

  /* Buffers are initially empty */
  for(int i=0; i<NR_BUFFERS; i+=1) {
    srd->buffer[i].used = 0;
  }
}
