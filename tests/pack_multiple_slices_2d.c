#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <msgpack.h>
#include <hdf5.h>

#include "verify.h"
#include "pack_dataset.h"
#include "decode_ndarray.h"
#include "pack_multiple_slices_2d.h"


void pack_multiple_slices_2d(hid_t dataset_id, int rank, int nx, int ny, int nr_slices,
                          hsize_t *start, hsize_t *count, bool succeeds, size_t buffer_size,
                          size_t max_size) {
  (void) nx;
  verify(rank==2);

  /* Set up a msgpack packer to pack to a memory buffer */
  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  /* Pack a slice of the dataset to the buffer */
  int result;
  if(max_size == 0)
    result = pack_dataset_multi_slice(dataset_id, rank, nr_slices, start, count, buffer_size, *pk);
  else
    result = pack_dataset_multi_slice_with_max_size(dataset_id, rank, nr_slices, start, count, buffer_size, *pk, max_size);

  /* Check if the return code matches our expectation of whether this case should work */
  verify((result==0) == succeeds);

  if(succeeds) {
    /* Now unpack the data */
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    verify(msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL) == MSGPACK_UNPACK_SUCCESS);

    /* Interpret the unpacked data as an ndarray */
    struct ndarray arr = decode_ndarray(msg.data);
    verify(arr.status==0);

    /* Compute expected size of result */
    hsize_t total_count[2] = {0, count[1]};
    if(nr_slices > 0) {
      for(int i=0; i<nr_slices; i+=1)
        total_count[0] += count[2*i+0];
    }

    /* Check array metadata */
    verify(arr.rank==2);
    verify(arr.shape[0] == total_count[0]);
    verify(arr.shape[1] == total_count[1]);

    /* Check array values */
    int *arr_data = (int *) arr.data;
    hsize_t row_nr = 0;
    /* Loop over requested slices */
    for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1) {
      /* Loop over elements in this slice */
      for(hsize_t i=0; i<count[2*slice_nr+0]; i+=1) {
        for(hsize_t j=0; j<count[2*slice_nr+1]; j+=1) {
        /* Get the value of this element from the decoded array */
          int unpacked_value = arr_data[j+total_count[1]*row_nr];
          /* Compute coordinates of this element in the full dataset */
          int input_i = i + start[2*slice_nr+0];
          int input_j = j + start[2*slice_nr+1];
          /* Compute the value we expect at these coordinates */
          int expected_value = input_j+10*ny*input_i;
          /* Check for agreement */
          verify(unpacked_value==expected_value);
        }
        row_nr += 1;
      }
    }
    msgpack_unpacked_destroy(&msg);
    if(arr.status==0)free(arr.data);
    printf("%d 2D slices with buffer size %d max size %d decoded ok\n", nr_slices, (int) buffer_size, (int) max_size);
  } else {
    printf("%d 2D slices with buffer size %d max size %d failed as expected\n", nr_slices, (int) buffer_size, (int) max_size);
  }

  /* Tidy up before the next slice */
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(pk);
}
