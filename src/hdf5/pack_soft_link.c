#define _POSIX_C_SOURCE 200809L
#include "pack_soft_link.h"
#include <string.h>
#include <hdf5.h>
#include <msgpack.h>
#include "verify.h"


/*
  Pack a soft link to a msgpack packer.

  A group is represented as a msgpack map:

  {
  "hdf5_object" : "soft_link",
  "target"  : path to the target object (string)
  }

  Returns 0 on success, <0 on failure.
  Packer may contain partially written data on failure.
*/
int pack_soft_link(hid_t obj_id, const char *name, msgpack_packer pk, const H5L_info2_t *link_info) {

  int result = -1;
  char *target = NULL;

  /* This should be a soft link */
  assert(link_info->type == H5L_TYPE_SOFT);

  /* Allocate storage for target path */
  target = malloc(link_info->u.val_size);

  /* Get the link target path */
  if(H5Lget_val(obj_id, name, target, link_info->u.val_size, H5P_DEFAULT) < 0)goto cleanup;
  size_t target_length = strnlen(target, link_info->u.val_size); /* length without null terminator */

  /* Serialize the soft link as a msgpack map */
  check(msgpack_pack_map(&pk, 2));

  /* Add entry to identify this as a group */
  check(msgpack_pack_str(&pk, 11));
  check(msgpack_pack_str_body(&pk, "hdf5_object", 11));
  check(msgpack_pack_str(&pk, 9));
  check(msgpack_pack_str_body(&pk, "soft_link", 9));

  /* Add an entry with the target path */
  check(msgpack_pack_str(&pk, 6));
  check(msgpack_pack_str_body(&pk, "target", 6));
  check(msgpack_pack_str(&pk, target_length));
  check(msgpack_pack_str_body(&pk, target, target_length));

  /* Success */
  result = 0;

 cleanup:
  if(target)free(target);
  return result;
}
