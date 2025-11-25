#include <stdlib.h>
#include <stdio.h>

#include <hdf5.h>
#include <msgpack.h>

#include "verify.h"
#include "hdfstream.h"
#include "decode_ndarray.h"
#include "receive_ndarray.h"
#include "receive_msgpack_object.h"

/*
  Receive a msgpack serialized ndarray and return type, dimensions and data in response struct.
  Need to free(result.data) afterwards.

  Only works for atomic data types where the numpy type is just a single string.
*/
struct ndarray receive_ndarray_slices(struct hdfstream *hs, char *filename,
                                      char *datasetname, int nr_slices,
                                      int rank, hsize_t *start,
                                      hsize_t *count, const size_t buffer_size) {

  struct ndarray result;
  result.status = -1;

  /* Open the data stream */
  struct data_stream *stream = hdfstream_dataset_multi_slice_open(hs, filename, datasetname, nr_slices,
                                                                  rank, start, count, buffer_size);
  if(!stream)return result;

  /* Receive the dataset slice object */
  msgpack_unpacker unp;
  msgpack_unpacked und;
  if(receive_msgpack_object(stream, &unp, &und, buffer_size) != 0) {
    msgpack_unpacker_destroy(&unp);
    hdfstream_close_stream(stream);
    return result;
  }
  msgpack_object obj = und.data;

  /* Interpret the result as an encoded ndarray */
  result = decode_ndarray(obj);
  verify(result.status==0);

  /* Should have unpacked all data */
  verify(msgpack_unpacker_next(&unp, &und) != MSGPACK_UNPACK_SUCCESS);

  /* Tidy up and return */
  msgpack_unpacker_destroy(&unp);
  hdfstream_close_stream(stream);

  return result;
}
