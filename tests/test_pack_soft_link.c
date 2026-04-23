#include <stdio.h>
#include <stdint.h>
#include <msgpack.h>
#include <hdf5.h>

#include "verify.h"
#include "verify_all_closed.h"
#include "pack_soft_link.h"
#include "create_test_file.h"

/*
  Check that we can correctly encode a soft link
*/
int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  printf("msgpack-c version is %s\n", MSGPACK_VERSION);

  /* Create a file for testing */
  hid_t file_id = create_file_in_memory();
  verify(file_id>=0);

  /* Create a group in the file */
  hid_t group_id = H5Gcreate(file_id, "Group", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  verify(group_id>=0);
  H5Gclose(group_id);

  /* Create a soft link to the group */
  verify(H5Lcreate_soft("Group", file_id, "LinkToGroup", H5P_DEFAULT, H5P_DEFAULT) >= 0);

  /* Set up a msgpack packer to pack to a memory buffer */
  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  /* Pack the soft link to the memory buffer */
  H5L_info2_t link_info;
  H5Lget_info(file_id, "LinkToGroup", &link_info, H5P_DEFAULT);
  verify(pack_soft_link(file_id, "LinkToGroup", *pk, &link_info) == 0);

  /* Now unpack the data from the buffer */
  msgpack_unpacked msg;
  msgpack_unpacked_init(&msg);
  verify(msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL) == MSGPACK_UNPACK_SUCCESS);

  /* Check the result: should be a map with two entries */
  msgpack_object obj = msg.data;
  verify(obj.type == MSGPACK_OBJECT_MAP);
  verify(obj.via.map.size == 2);

  /* First entry should be "hdf5_object" : "soft_link" */
  verify(obj.via.map.ptr[0].key.type == MSGPACK_OBJECT_STR);
  verify(strncmp(obj.via.map.ptr[0].key.via.str.ptr, "hdf5_object", obj.via.map.ptr[0].key.via.str.size) == 0);
  verify(obj.via.map.ptr[0].val.type == MSGPACK_OBJECT_STR);
  verify(strncmp(obj.via.map.ptr[0].val.via.str.ptr, "soft_link", obj.via.map.ptr[0].val.via.str.size) == 0);

  /* First entry should be "target" : "Group" */
  verify(obj.via.map.ptr[1].key.type == MSGPACK_OBJECT_STR);
  verify(strncmp(obj.via.map.ptr[1].key.via.str.ptr, "target", obj.via.map.ptr[1].key.via.str.size) == 0);
  verify(obj.via.map.ptr[1].val.type == MSGPACK_OBJECT_STR);
  verify(strncmp(obj.via.map.ptr[1].val.via.str.ptr, "Group", obj.via.map.ptr[1].val.via.str.size) == 0);

  /* Tidy up */
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(pk);
  msgpack_unpacked_destroy(&msg);
  H5Fclose(file_id);
  verify_all_closed();
  return 0;
}
