#ifndef RECEIVE_MSGPACK_OBJECT_H
#define RECEIVE_MSGPACK_OBJECT_H

#include <msgpack.h>
#include "hdfstream.h"

int receive_msgpack_object(struct data_stream *stream, msgpack_unpacker *unp, msgpack_unpacked *und,
                           const size_t buffer_size);

#endif
