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
  Check that we can correctly encode a "slice" (i.e. all) of a scalar dataset
*/
int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  printf("msgpack-c version is %s\n", MSGPACK_VERSION);

  /* Create a file for testing */
  hid_t file_id = create_file_in_memory();
  verify(file_id>=0);

  /* Create a dataset and write to it */
  int data = 1503238553;
  hid_t dspace_id = H5Screate(H5S_SCALAR);
  hid_t dataset_id = H5Dcreate(file_id, "int_scalar", H5T_NATIVE_INT,
                               dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data);
  H5Sclose(dspace_id);

  /* Set up a msgpack packer to pack to a memory buffer */
  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  /* Pack a slice of the dataset to the buffer */
  hsize_t start[1] = {0};
  hsize_t count[1] = {0};
  verify(pack_dataset_slice(dataset_id, /*rank=*/ 0, start, count, /*buffer_size=*/ 1024, *pk) == 0);

  /* Now unpack the data */
  msgpack_unpacked msg;
  msgpack_unpacked_init(&msg);
  verify(msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL) == MSGPACK_UNPACK_SUCCESS);

  /* Interpret the unpacked data as an ndarray */
  struct ndarray arr = decode_ndarray(msg.data);
  verify(arr.status==0);

  /* Check array metadata */
  verify(arr.rank==0);

  /* Check array contents */
  int *scalar = (int *) arr.data;
  verify(*scalar == data);

  /* Tidy up */
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(pk);
  msgpack_unpacked_destroy(&msg);
  if(arr.status==0)free(arr.data);

  H5Dclose(dataset_id);
  H5Fclose(file_id);
  verify_all_closed();
  return 0;
}
