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
  Check that we can correctly encode a slice of a 1D dataset
*/
int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  printf("msgpack-c version is %s\n", MSGPACK_VERSION);

  /* Create a file for testing */
  hid_t file_id = create_file_in_memory();
  verify(file_id>=0);

  /* Create a 1D int array */
  const int nr_elements = 10000;
  int *dsdata = malloc(sizeof(int)*nr_elements);
  for(int i=0; i<nr_elements; i+=1)
    dsdata[i] = i;

  /* Create a dataset and write the array to it */
  hsize_t dims[1] = {(hsize_t) nr_elements};
  hid_t dspace_id = H5Screate_simple(1, dims, NULL);
  hid_t dataset_id = H5Dcreate(file_id, "int_data_1d", H5T_NATIVE_INT,
                               dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dsdata);
  free(dsdata);
  H5Sclose(dspace_id);

  /* Slices to try reading */
  int nr_slices = 9;
  hsize_t starts[] = {0, 0, 0,     0,    1024, 1023, 1375, 9879, 10000};
  hsize_t counts[] = {0, 1, 10000, 1024, 1024, 1024, 4395, 0,    0};

  for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1) {

    /* Set up a msgpack packer to pack to a memory buffer */
    msgpack_sbuffer *buffer = msgpack_sbuffer_new();
    msgpack_packer *pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

    /* Pack a slice of the dataset to the buffer */
    hsize_t start[1] = {starts[slice_nr]};
    hsize_t count[1] = {counts[slice_nr]};
    verify(pack_dataset_slice(dataset_id, /*rank=*/ 1, start, count, /*buffer_size=*/ 1024, *pk) == 0);

    /* Now unpack the data */
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    verify(msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL) == MSGPACK_UNPACK_SUCCESS);

    /* Interpret the unpacked data as an ndarray */
    struct ndarray arr = decode_ndarray(msg.data);
    verify(arr.status==0);

    /* Check array metadata */
    verify(arr.rank==1);
    verify(arr.shape[0] == count[0]);

    /* Check array contents */
    for(hsize_t i=0; i<count[0]; i+=1) {
      int *data = (int *) arr.data;
      verify(data[i] == ((int) (start[0]+i)));
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
