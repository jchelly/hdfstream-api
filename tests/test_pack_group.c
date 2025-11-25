#include <stdio.h>
#include <stdint.h>
#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <hdf5.h>

#include "verify.h"
#include "pack_group.h"
#include "create_test_file.h"


static void fill_data(int rank, hsize_t *dims, void *data) {

  int *ptr = (int *) data;

  size_t nr_elements = 1;
  for(int i=0; i<rank; i+=1)
    nr_elements *= dims[i];

  for(size_t i=0; i<nr_elements; i+=1)
    ptr[i] = (int) i;
}


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  /* Create HDF5 file */
  hid_t file_id = create_file_in_memory();

  /* Create some nested groups */
  hid_t grp_id = H5Gcreate(file_id, "group", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  hid_t sg1_id = H5Gcreate(file_id, "group/subgroup1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  hid_t sg2_id = H5Gcreate(file_id, "group/subgroup2", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  hid_t ss1_id = H5Gcreate(file_id, "group/subgroup2/subsubgroup1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Add an attribute to a group */
  {
    hsize_t dims[] = {0};
    int data = 100;
    create_attribute(ss1_id, "subsub_attr_int_scalar", 0, dims, H5T_NATIVE_INT, &data);
  }

  /* Create a dataset with attributes */
  {
    hsize_t dims[] = {10};
    create_dataset(file_id, "group/subgroup1/test_dataset", 1, dims, H5T_NATIVE_INT, fill_data);
  }
  hid_t dataset_id = H5Dopen(file_id, "group/subgroup1/test_dataset", H5P_DEFAULT);
  {
    hsize_t dims[] = {0};
    int data = 100;
    create_attribute(dataset_id, "dataset_attr_int_scalar", 0, dims, H5T_NATIVE_INT, &data);
  }
  {
    hsize_t dims[] = {3,3};
    double data[] = {0.,1.,2.,3.,4.,5.,6.,7.,8.};
    create_attribute(dataset_id, "dataset_attr_double_2d", 2, dims, H5T_NATIVE_DOUBLE, &data);
  }

  const size_t buffer_size = 1024;

  /* Pack everything to a file */
  msgpack_packer pk;
  FILE *fd = create_temp_fd();
  msgpack_packer_init(&pk, fd, msgpack_fbuffer_write);
  verify(pack_group(file_id, pk, 10, SIZE_MAX, buffer_size) == 0);
  fclose(fd);

  /* Try again with recursion limit */
  fd = fopen("recursive_groups_limit_recursion.dat", "wb");
  msgpack_packer_init(&pk, fd, msgpack_fbuffer_write);
  verify(pack_group(file_id, pk, 1, SIZE_MAX, buffer_size) == 0);
  fclose(fd);

  H5Gclose(grp_id);
  H5Gclose(sg1_id);
  H5Gclose(sg2_id);
  H5Gclose(ss1_id);

  H5Dclose(dataset_id);
  H5Fclose(file_id);

  return 0;
}
