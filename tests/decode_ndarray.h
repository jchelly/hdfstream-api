#ifndef DECODE_NDARRAY_H
#define DECODE_NDARRAY_H

#include <hdf5.h>
#include <msgpack.h>

#define MAX_DIMS 32
#define MAX_TYPE_LEN 1023


struct ndarray {
  int status; /* 0 = success */
  int rank;
  hsize_t shape[MAX_DIMS];
  char type[MAX_TYPE_LEN+1];
  size_t data_len;
  void *data;
};

struct ndarray decode_ndarray(msgpack_object obj);

#endif
