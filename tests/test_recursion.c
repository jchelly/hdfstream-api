#include <stdio.h>
#include <stdint.h>
#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <hdf5.h>

#include "verify.h"
#include "pack_group.h"
#include "create_test_file.h"
#include "decoder.h"
#include "pack_and_decode.h"

static void fill_data(int rank, hsize_t *dims, void *data) {

  int *ptr = (int *) data;

  size_t nr_elements = 1;
  for(int i=0; i<rank; i+=1)
    nr_elements *= dims[i];

  for(size_t i=0; i<nr_elements; i+=1)
    ptr[i] = (int) i;
}

static hid_t create_test_file(void) {

  /* Create HDF5 file */
  hid_t file_id = create_file_in_memory();

  /* Create some nested groups */
  hid_t grp_id = H5Gcreate(file_id, "group1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  hid_t sg1_id = H5Gcreate(file_id, "group1/subgroup1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  hid_t sg2_id = H5Gcreate(file_id, "group1/subgroup2", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  hid_t ss1_id = H5Gcreate(file_id, "group1/subgroup2/subsubgroup1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Add an attribute to a group */
  {
    hsize_t dims[] = {0};
    int data = 100;
    create_attribute(ss1_id, "subsub_attr_int_scalar", 0, dims, H5T_NATIVE_INT, &data);
  }

  /* Create a dataset with attributes */
  {
    hsize_t dims[] = {10};
    create_dataset(file_id, "group1/subgroup1/test_dataset", 1, dims, H5T_NATIVE_INT, fill_data);
  }
  hid_t dataset_id = H5Dopen(file_id, "group1/subgroup1/test_dataset", H5P_DEFAULT);
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
  H5Gclose(grp_id);
  H5Gclose(sg1_id);
  H5Gclose(sg2_id);
  H5Gclose(ss1_id);
  H5Dclose(dataset_id);

  return file_id;
}


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  int max_depth = 10;
  size_t data_size_limit = SIZE_MAX;
  size_t buffer_size = 1024;

  /* Create a HDF5 file */
  hid_t file_id = create_test_file();

  /* Serialize and interpret file contents */
  hs_object root = pack_and_decode(file_id, max_depth, data_size_limit, buffer_size);
  verify(root.type != HS_NULL);

  /* Check we have the expected structure */
  verify(root.type == HS_GROUP);
  /* Root should contain one group called "group" */
  verify(root.group.nr_members==1);
  verify(hs_get_member(root, "group1").type != HS_NULL);
  verify(root.group.member_object[0].type == HS_GROUP);

  /* Free the decoded data */
  hs_free_object(&root);

  /* Close the file */
  H5Fclose(file_id);

  return 0;
}
