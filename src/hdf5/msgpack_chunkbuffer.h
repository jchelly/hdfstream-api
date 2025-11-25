#ifndef MSGPCK_CHUNKBUFFER_H
#define MSGPCK_CHUNKBUFFER_H
/*
  msgpack buffer which writes output in chunks prefixed with their
  size as a size_t.
*/

#include "shared_data.h"

#define MSGPACK_CHUNKBUFFER_OK 0
#define MSGPACK_CHUNKBUFFER_ERROR -1

struct msgpack_chunkbuffer {
  char *shared_data;
  struct shared_reader_data *srd;
  int buffer_nr;
  size_t buffer_size;
};

int msgpack_chunkbuffer_init(struct msgpack_chunkbuffer *cb, size_t buffer_size,
                             void *shared_data, struct shared_reader_data *srd);

int msgpack_chunkbuffer_write(void *data, const char *buf, size_t len);

int msgpack_chunkbuffer_destroy(struct msgpack_chunkbuffer *cb, int return_code);

int msgpack_chunkbuffer_flush(struct msgpack_chunkbuffer *cb);

#endif
