#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <msgpack.h>
#include <hdf5.h>

#include "verify.h"
#include "pack_dataset.h"
#include "decode_ndarray.h"
#include "pack_multiple_slices_1d.h"


void pack_multiple_slices_1d(hid_t dataset_id, int rank, int nr_slices, hsize_t *start, hsize_t *count, bool succeeds,
			     size_t buffer_size, size_t max_size) {

  /* Set up a msgpack packer to pack to a memory buffer */
  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  /* Pack a slice of the dataset to the buffer */
  int result;
  if(max_size > 0) {
    result = pack_dataset_multi_slice_with_max_size(dataset_id, rank, nr_slices, start, count, buffer_size, *pk, max_size);
  } else {
    result = pack_dataset_multi_slice(dataset_id, rank, nr_slices, start, count, buffer_size, *pk);
  }
  verify((result==0) == succeeds);

  hsize_t total_count = 0;
  if(succeeds) {

    /* Now unpack the data */
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    verify(msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL) == MSGPACK_UNPACK_SUCCESS);

    /* Interpret the unpacked data as an ndarray */
    struct ndarray arr = decode_ndarray(msg.data);
    verify(arr.status==0);

    /* Compute expected size of result */
    if(nr_slices > 0) {
      for(int i=0; i<nr_slices; i+=1)
	total_count += count[i];
    }

    /* Check array metadata */
    verify(arr.rank==1);
    verify(arr.shape[0] == total_count);

    /* Check array values */
    hsize_t row_nr = 0;
    for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1) {
      for(hsize_t i=0; i<count[slice_nr]; i+=1) {
	int *arr_data = (int *) arr.data;
	verify(arr_data[row_nr] == (int) (start[slice_nr]+i));
	row_nr += 1;
      }
    }

    msgpack_unpacked_destroy(&msg);
    free(arr.data);
  }

  /* Tidy up before the next slice */
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(pk);
  printf("%d elements in %d slices with buffer size %d max size %d decoded ok\n", (int) total_count, nr_slices, (int) buffer_size, (int) max_size);

}
