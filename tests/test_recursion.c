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

static const int group_depth = 10;


static hid_t create_test_file(void) {

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
  for(int i=0; i<group_depth; i+=1)
    H5Gclose(grp_id[i]);

  return file_id;
}


static void run_test(int max_depth) {

  size_t data_size_limit = SIZE_MAX;
  size_t buffer_size = 1024;

  /* Create a HDF5 file */
  hid_t file_id = create_test_file();

  /* Serialize and interpret file contents */
  hs_object root = pack_and_decode(file_id, max_depth, data_size_limit, buffer_size);

  /* We should always have the root group with one member */
  verify(root.type != HS_NULL);
  verify(root.group.nr_members==1);

  /* Check that the recursion depth parameter took effect */
  hs_object obj = root;
  int depth = -1;
  while(obj.type != HS_NULL) {
    verify(obj.type == HS_GROUP);
    if(obj.group.nr_members > 0) {
      verify(strcmp("groupname", obj.group.member_name[0]) == 0);
      obj = obj.group.member_object[0];
      depth += 1;
    } else {
      /* No more nested groups */
      break;
    }
  }
  int expected_depth = (max_depth < group_depth-1) ? max_depth : group_depth-1;
  verify(depth==expected_depth);
  
  /* Free the decoded data */
  hs_free_object(&root);

  /* Close the file */
  H5Fclose(file_id);  
}


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  for(int max_depth=0; max_depth<15; max_depth +=1)
    run_test(max_depth);

  return 0;
}
