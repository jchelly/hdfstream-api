#include <stdio.h>
#include <stdint.h>
#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <hdf5.h>

#include "verify.h"
#include "verify_all_closed.h"
#include "pack_dataset.h"
#include "create_test_file.h"
#include "decode_ndarray.h"

/*
  Check that we can correctly encode a slice of a 2D dataset
*/
int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  printf("msgpack-c version is %s\n", MSGPACK_VERSION);

  /* Create a file for testing */
  hid_t file_id = create_file_in_memory();
  verify(file_id>=0);

  /* Create a 2D int array */
  const int nx = 2000;
  const int ny = 1000;
  int *data = malloc(sizeof(int)*nx*ny);
  for(int i=0; i<nx; i+=1) {
    for(int j=0; j<ny; j+=1) {
      data[j+i*ny] = 10000*j + i;
    }
  }

  /* Create a dataset and write the array to it */
  hsize_t dims[2] = {(hsize_t) nx, (hsize_t) ny};
  hid_t dspace_id = H5Screate_simple(2, dims, NULL);
  hid_t dataset_id = H5Dcreate(file_id, "int_data_2d", H5T_NATIVE_INT,
                               dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  free(data);
  H5Sclose(dspace_id);

  /* A few different slices, including some edge cases */
  hsize_t starts[][2] = {{0,0}, {0,  0}, {0,  0}, {nx, ny}, {nx-1, ny-1}, {300, 0},  {0,  300}, {100, 200}, {953, 511}};
  hsize_t counts[][2] = {{0,0}, {10, 0}, {0, 10}, {0,  0},  {1,    1},    {600, ny}, {nx, 600}, {700, 500}, {143, 97}};
  int nr_slices = sizeof(starts) / sizeof(starts[0]);

  for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1) {

    hsize_t start[] = {starts[slice_nr][0], starts[slice_nr][1]};
    hsize_t count[] = {counts[slice_nr][0], counts[slice_nr][1]};

    /* Set up a msgpack packer to pack to a memory buffer */
    msgpack_sbuffer *buffer = msgpack_sbuffer_new();
    msgpack_packer *pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

    /* Pack a slice of the dataset to the buffer. Buffer must hold data for at least one row. */
    verify(pack_dataset_slice(dataset_id, /*rank=*/ 2, start, count, /*buffer_size=*/ 10*ny*sizeof(int), *pk) == 0);

    /* Now unpack the data */
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    verify(msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL) == MSGPACK_UNPACK_SUCCESS);

    /* Interpret the unpacked data as an ndarray */
    struct ndarray arr = decode_ndarray(msg.data);
    verify(arr.status==0);

    /* Check array metadata */
    verify(arr.rank==2);
    verify(arr.shape[0] == count[0]);
    verify(arr.shape[1] == count[1]);

    /* Check values */
    for(int i=0; i<((int) count[0]); i+=1) {
      for(int j=0; j<((int) count[1]); j+=1) {
        int expected = 10000*(j+start[1])+(i+start[0]);
        int index = j + i*count[1];
        verify(((int *) arr.data)[index]==expected);
      }
    }

    /* Tidy up before the next slice */
    msgpack_sbuffer_free(buffer);
    msgpack_packer_free(pk);
    msgpack_unpacked_destroy(&msg);
    if(arr.status==0)free(arr.data);

    printf("Slice %d ok\n", slice_nr);
  }

  H5Dclose(dataset_id);
  H5Fclose(file_id);
  verify_all_closed();
  return 0;
}
