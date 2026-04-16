#include <stdio.h>
#include <stdint.h>
#include <string.h>
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


static hid_t create_test_file(int group_depth, int nr_datasets) {

  /* Create HDF5 file */
  hid_t file_id = create_file_in_memory();

  /* Create some nested groups */
  hid_t grp_id[group_depth];
  for(int i=0; i<group_depth; i+=1) {
    if(i==0) {
      grp_id[i] = H5Gcreate(file_id, "groupname", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    } else {
      grp_id[i] = H5Gcreate(grp_id[i-1], "groupname", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }
  }

  /* Create some datasets in the innermost group */
  hid_t loc_id = grp_id[group_depth-1];
  for(int i=0; i<nr_datasets; i+=1) {
    char name[500];
    sprintf(name, "Dataset%d", i);
    hsize_t dims[] = {10};
    create_dataset(loc_id, name, 1, dims, H5T_NATIVE_INT, fill_data);
  }

  for(int i=0; i<group_depth; i+=1)
    H5Gclose(grp_id[i]);

  return file_id;
}


static void run_test(int max_depth, int group_depth, int nr_datasets) {

  size_t data_size_limit = SIZE_MAX;
  size_t buffer_size = 1024;

  printf("max_depth=%d, group_depth=%d, nr_datasets=%d\n", max_depth, group_depth, nr_datasets);

  /* Create a HDF5 file */
  hid_t file_id = create_test_file(group_depth, nr_datasets);

  /* Serialize and interpret file contents */
  hs_object root = pack_and_decode(file_id, max_depth, data_size_limit, buffer_size);

  /* We should always have the root group with one member */
  verify(root.type != HS_NULL);
  verify(root.group.nr_members==1);

  /* Check that the recursion depth parameter took effect */
  hs_object obj = root;
  hs_object deepest_group = root;
  int depth = 0;
  while(obj.type == HS_GROUP) {
    verify(obj.type == HS_GROUP);
    if(obj.group.nr_members > 0) {
      obj = hs_get_member(obj, "groupname");
      if(obj.type == HS_GROUP) {
        printf("Found group at depth %d\n", depth);
        depth += 1;
        deepest_group = obj;
      }
    } else {
      /* No more nested groups */
      break;
    }
  }
  int expected_depth = (max_depth < group_depth) ? max_depth : group_depth;
  printf("depth=%d, expected_depth=%d\n", depth, expected_depth);
  verify(depth==expected_depth);

  printf("Deepest group members: %d\n", deepest_group.group.nr_members);
  for(int i=0; i<deepest_group.group.nr_members; i+=1) {
    printf("  Member: %s, type=%d\n", deepest_group.group.member_name[i], deepest_group.group.member_object[i].type);
  }

  /* Check that the datasets exist in the deepest group */
  for(int dataset_nr=0; dataset_nr<nr_datasets; dataset_nr+=1) {
    char name[500];
    sprintf(name, "Dataset%d", dataset_nr);
    hs_object ds = hs_get_member(deepest_group, name);
    if(max_depth == group_depth) {
      /* Should have dataset names but no metadata */
      assert(ds.type==HS_NULL);
    } else if(max_depth > group_depth) {
      /* Should have returned the datasets */
      assert(ds.type==HS_DATASET);
    } else {
      /* Recursion depth did not reach the datasets */
      assert(deepest_group.group.nr_members==1);
      assert(deepest_group.group.member_object[0].type==HS_NULL);
      assert(ds.type==HS_ERROR);
    }
  }

  /* Free the decoded data */
  hs_free_object(&root);

  /* Close the file */
  H5Fclose(file_id);
}


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  for(int max_depth=0; max_depth<15; max_depth +=1)
    run_test(max_depth, 10, 5);

  return 0;
}
