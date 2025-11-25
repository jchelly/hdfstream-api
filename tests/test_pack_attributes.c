#include <stdio.h>
#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <hdf5.h>

#include "verify.h"
#include "pack_attributes.h"
#include "create_test_file.h"

int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  /* Create HDF5 file */
  hid_t file_id = create_file_in_memory();

  /* Create a group */
  hid_t group_id = H5Gcreate(file_id, "TestGroup", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Add some attributes */
  {
    hsize_t dims[] = {0};
    int data = 100;
    create_attribute(group_id, "int_scalar", 0, dims, H5T_NATIVE_INT, &data);
  }
  {
    hsize_t dims[] = {5};
    int data[] = {0,1,2,3,4};
    create_attribute(group_id, "int_1d", 1, dims, H5T_NATIVE_INT, &data);
  }
  {
    hsize_t dims[] = {3,3};
    double data[] = {0.,1.,2.,3.,4.,5.,6.,7.,8.};
    create_attribute(group_id, "double_2d", 2, dims, H5T_NATIVE_DOUBLE, &data);
  }

  /* Pack group object's attributes to a file */
  msgpack_packer pk;
  FILE *fd = create_temp_fd();
  msgpack_packer_init(&pk, fd, msgpack_fbuffer_write);
  pack_attributes(group_id, pk);
  fclose(fd);

  H5Fclose(file_id);

  return 0;
}
