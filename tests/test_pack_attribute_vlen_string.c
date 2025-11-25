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

  /* Write a scalar vlen string attribute */
  {
    hsize_t dims[] = {0};
    char *data = "This is a variable length string";
    hid_t dtype_id = H5Tcreate(H5T_STRING, H5T_VARIABLE);
    create_attribute(group_id, "vlen_str_scalar", 0, dims, dtype_id, &data);
    H5Tclose(dtype_id);
  }

  /* Write a 1D array vlen string attribute */
  {
    const hsize_t n = 10;
    hsize_t dims[1] = {n};
    const size_t max_str_len = 1024;
    char *data[n];
    for(hsize_t i=0; i<n; i+=1) {
      data[i] = malloc(max_str_len);
      sprintf(data[i], "This is string %d of the 1D array", (int) i);
    }

    hid_t dtype_id = H5Tcreate(H5T_STRING, H5T_VARIABLE);
    create_attribute(group_id, "vlen_str_array_1d", 1, dims, dtype_id, &data);
    H5Tclose(dtype_id);

    for(hsize_t i=0; i<n; i+=1)
      free(data[i]);
  }

  /* Write a 2D array vlen string attribute */
  {
    const hsize_t n = 10;
    hsize_t dims[2] = {n/2,2};
    const size_t max_str_len = 1024;
    char *data[n];
    for(hsize_t i=0; i<n; i+=1) {
      data[i] = malloc(max_str_len);
      sprintf(data[i], "This is string %d of the 2D array", (int) i);
    }

    hid_t dtype_id = H5Tcreate(H5T_STRING, H5T_VARIABLE);
    create_attribute(group_id, "vlen_str_array_2d", 2, dims, dtype_id, &data);
    H5Tclose(dtype_id);

    for(hsize_t i=0; i<n; i+=1)
      free(data[i]);
  }


  /* Pack group object's attributes to a file */
  msgpack_packer pk;
  FILE *fd = create_temp_fd();
  msgpack_packer_init(&pk, fd, msgpack_fbuffer_write);
  verify(pack_attributes(group_id, pk)==0);
  fclose(fd);

  H5Fclose(file_id);

  return 0;
}
