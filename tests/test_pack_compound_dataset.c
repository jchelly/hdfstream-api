#include <stdio.h>
#include <stdint.h>
#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <hdf5.h>

#include "verify.h"
#include "pack_dataset.h"
#include "create_test_file.h"
#include "decode_ndarray.h"

/*
  Check that we can correctly encode a dataset of compound type
*/
int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  printf("msgpack-c version is %s\n", MSGPACK_VERSION);

  /* Create a file for testing */
  hid_t file_id = create_file_in_memory();
  verify(file_id>=0);

  /* Create test data */
  struct test_data_t {
    int8_t  i8;
    int32_t i32;
  };
  const int nr_elements = 100;
  struct test_data_t *dsdata = malloc(sizeof(struct test_data_t)*nr_elements);
  for(int i=0; i<nr_elements; i+=1) {
    dsdata[i].i8 = i;
    dsdata[i].i32 = 1000*i;
  }

  /* Create the HDF5 data type corresponding to the struct */
  hid_t dtype_id = H5Tcreate(H5T_COMPOUND, sizeof(struct test_data_t));
  H5Tinsert(dtype_id, "i8", HOFFSET(struct test_data_t, i8), H5T_NATIVE_INT8);
  H5Tinsert(dtype_id, "i32", HOFFSET(struct test_data_t, i32), H5T_NATIVE_INT32);

  /* Create a dataset and write the array to it */
  hsize_t dims[1] = {(hsize_t) nr_elements};
  hid_t dspace_id = H5Screate_simple(1, dims, NULL);
  hid_t dataset_id = H5Dcreate(file_id, "compound_data_1d", dtype_id,
                               dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Dwrite(dataset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, dsdata);
  free(dsdata);
  H5Sclose(dspace_id);
  H5Tclose(dtype_id);

  /* Serialize the dataset to a memory buffer */
  hsize_t start[] = {0};
  hsize_t count[] = {nr_elements};
  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);
  verify(pack_dataset_slice(dataset_id, /*rank=*/ 1, start, count, /*buffer_size=*/ 1024, *pk) == 0);

  /* Now deserialize the data from the buffer */
  msgpack_unpacked msg;
  msgpack_unpacked_init(&msg);
  verify(msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL) == MSGPACK_UNPACK_SUCCESS);

  /* Interpret the unpacked data as an ndarray */
  struct ndarray arr = decode_ndarray(msg.data);
  verify(arr.status==0);

  /* Check the body of the array */
  size_t packed_size = sizeof(int8_t)+sizeof(int32_t);
  verify(arr.data_len == count[0]*packed_size);

  /* Check the values */
  for(int i=0; i<((int) count[0]); i+=1) {

    /* Find the data for this element */
    char *ptr = ((char *) arr.data) + i*packed_size;

    /* Unpack and check fields */
    int8_t i8;
    memcpy(&i8, ptr, sizeof(int8_t));
    verify(i8 == i);
    int32_t i32;
    memcpy(&i32, ptr+sizeof(int8_t), sizeof(int32_t));
    verify(i32 == i*1000);
  }

  /* Tidy up */
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(pk);
  msgpack_unpacked_destroy(&msg);
  free(arr.data);

  H5Fclose(file_id);
  return 0;
}
