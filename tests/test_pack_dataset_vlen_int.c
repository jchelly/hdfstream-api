#include <stdio.h>
#include <stdint.h>
#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <hdf5.h>

#include "verify.h"
#include "verify_all_closed.h"
#include "pack_dataset.h"
#include "create_test_file.h"

int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  printf("msgpack-c version is %s\n", MSGPACK_VERSION);

  /* Create a file for testing */
  hid_t file_id = create_file_in_memory();
  verify(file_id>=0);

  /* Create an array of 1D arrays of integers, of varying length */
  const int nr_arrays = 10;
  hvl_t data[nr_arrays];
  for(int i=0; i<nr_arrays; i+=1) {
    data[i].len = (size_t) i;
    data[i].p = malloc(sizeof(int) * data[i].len);
    for(size_t j=0; j<data[i].len; j+=1) {
      int *iptr = (int *) data[i].p;
      iptr[j] = j+1000*i;
    }
  }

  /* Create a dataset and write the array to it */
  hsize_t dims[1] = {(hsize_t) nr_arrays};
  hid_t dspace_id = H5Screate_simple(1, dims, NULL);
  hid_t dtype_id = H5Tvlen_create(H5T_NATIVE_INT);
  hid_t dataset_id = H5Dcreate(file_id, "vlen_int", dtype_id, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Dwrite(dataset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  for(int i=0; i<nr_arrays; i+=1)
    free(data[i].p);
  H5Tclose(dtype_id);
  H5Sclose(dspace_id);

  /* Set up a msgpack packer */
  msgpack_packer pk;
  FILE *fd = create_temp_fd();
  msgpack_packer_init(&pk, fd, msgpack_fbuffer_write);

  /* Pack the dataset contents to a file */
  size_t buffer_size = 128; /* Unrealistically small so we write multiple chunks */
  verify(pack_dataset(dataset_id, pk, /* data_size_limit = */ SIZE_MAX, buffer_size) == 0);
  fclose(fd);

  H5Dclose(dataset_id);
  H5Fclose(file_id);
  verify_all_closed();
  return 0;
}
