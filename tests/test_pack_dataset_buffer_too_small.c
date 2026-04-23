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

  hsize_t start[] = {0, 0};
  hsize_t count[] = {1500, 900};

  /* Set up a msgpack packer to pack to a memory buffer */
  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  /* Pack a slice of the dataset to the buffer. Should fail because a row doesn't fit in the buffer */
  verify(pack_dataset_slice(dataset_id, /*rank=*/ 2, start, count, /*buffer_size=*/ 512, *pk) != 0);

  /* Tidy up  */
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(pk);

  H5Dclose(dataset_id);
  H5Fclose(file_id);
  verify_all_closed();
  return 0;
}
