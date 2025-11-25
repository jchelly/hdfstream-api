#include "receive_msgpack_object.h"

#include <stdlib.h>
#include <stdio.h>
#include <msgpack.h>

#include "verify.h"
#include "hdfstream.h"

/*
  Receive a stream containing a single msgpack object.

  This initialises the msgpack objects unp and obj.

  Calling program must call msgpack_unpacker_destroy() on the unpacker
  to free allocated memory.

  This does not free the hdfstream object.
*/
int receive_msgpack_object(struct data_stream *stream, msgpack_unpacker *unp, msgpack_unpacked *und,
                           const size_t buffer_size) {

  /* Create msgpack unpacker to receive the data */
  msgpack_unpacker_init(unp, buffer_size);
  msgpack_unpacker_reserve_buffer(unp, buffer_size);

  /* Stream the object to the msgpack unpacker */
  char *ptr = msgpack_unpacker_buffer(unp);
  size_t bytes_read;
  size_t total_bytes_read = 0;
  int status;
  while((bytes_read = hdfstream_read_chunk(stream, ptr, &status))) {
    verify(bytes_read<=buffer_size);
    total_bytes_read += bytes_read;
    msgpack_unpacker_reserve_buffer(unp, total_bytes_read+buffer_size); // TODO: better buffer size strategy!
    ptr = msgpack_unpacker_buffer(unp) + total_bytes_read; // Buffer may have been reallocated
  }
  if(status != 0) {
    return -1;
  };
  msgpack_unpacker_buffer_consumed(unp, total_bytes_read);

  /* Unpack the response */
  msgpack_unpacked_init(und);
  msgpack_unpack_return ret;
  ret = msgpack_unpacker_next(unp, und);
  if(ret==MSGPACK_UNPACK_SUCCESS) {
    verify(unp->off==unp->used);
    return 0;
  } else {
    return -1;
  }
}
