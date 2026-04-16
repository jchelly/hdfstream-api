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


static hid_t create_test_file(void) {

  /* Create HDF5 file */
  hid_t file_id = create_file_in_memory();

  /* Create a group */
  hid_t grp_id = H5Gcreate(file_id, "Group", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Write a data type to the group */
  hid_t dtype_id = H5Tcopy(H5T_NATIVE_INT);
  H5Tcommit2(grp_id, "Type", dtype_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Tidy up */
  H5Tclose(dtype_id);
  H5Gclose(grp_id);

  return file_id;
}


static void run_test(void) {

  int max_depth = 5;
  size_t data_size_limit = SIZE_MAX;
  size_t buffer_size = 1024;

  /* Create a HDF5 file */
  hid_t file_id = create_test_file();

  /* Serialize and interpret file contents */
  hs_object root = pack_and_decode(file_id, max_depth, data_size_limit, buffer_size);

  /* We should always have the root group with one member */
  verify(root.type != HS_NULL);
  verify(root.group.nr_members==1);
  hs_object group = root.group.member_object[0];
  verify(group.type==HS_GROUP);
  hs_object dtype = group.group.member_object[0];
  verify(dtype.type==HS_UNKNOWN);

  /* Free the decoded data */
  hs_free_object(&root);

  /* Close the file */
  H5Fclose(file_id);
}


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  run_test();

  return 0;
}
