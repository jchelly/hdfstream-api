#include <stdio.h>
#include <stdint.h>
#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <hdf5.h>

#include "verify.h"
#include "pack_dataset.h"
#include "create_test_file.h"

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

  /* Add some attributes to the dataset */
  {
    hsize_t attr_dims[] = {0};
    int data = 100;
    create_attribute(dataset_id, "int_scalar", 0, attr_dims, H5T_NATIVE_INT, &data);
  }
  {
    hsize_t attr_dims[] = {5};
    int data[] = {0,1,2,3,4};
    create_attribute(dataset_id, "int_1d", 1, attr_dims, H5T_NATIVE_INT, &data);
  }
  {
    hsize_t attr_dims[] = {3,3};
    double data[] = {0.,1.,2.,3.,4.,5.,6.,7.,8.};
    create_attribute(dataset_id, "double_2d", 2, attr_dims, H5T_NATIVE_DOUBLE, &data);
  }

  /* Set up a msgpack packer */
  msgpack_packer pk;
  FILE *fd = create_temp_fd();
  msgpack_packer_init(&pk, fd, msgpack_fbuffer_write);

  /* Pack the dataset contents to a file */
  size_t buffer_size = 1024; /* Unrealistically small so we write multiple chunks */
  verify(pack_dataset(dataset_id, pk, /* data_size_limit = */ SIZE_MAX, buffer_size) == 0);
  fclose(fd);

  return 0;
}
