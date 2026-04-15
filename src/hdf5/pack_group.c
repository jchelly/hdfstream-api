#include <stdlib.h>
#include <string.h>
#include <hdf5.h>
#include <msgpack.h>

#include "pack_group.h"
#include "pack_dataset.h"
#include "pack_object.h"
#include "pack_unknown.h"
#include "pack_attributes.h"
#include "pack_soft_link.h"
#include "type_mapping.h"
#include "verify.h"

#if !H5_VERSION_GE(1, 12, 0)
#error "HDF5 version 1.12.0 or newer is required."
#endif

/* If a group contains too many links, just return the link names */
#define MAX_LINKS_FOR_RECURSION 20

/* Information needed to encode link targets */
struct visit_info {
  int encode_members;
  int depth;
  int max_depth;
  size_t data_size_limit;
  size_t buffer_size;
  msgpack_packer *pk;
};

static herr_t visit_link(hid_t obj_id, const char *name, const H5L_info2_t *info, void *op_data) {

  /* Negative return value from the H5Literate callback indicates an error */
  herr_t result = -1;

  /* Get a pointer to the struct with parameters we need */
  struct visit_info *vi = (struct visit_info *) op_data;
  msgpack_packer pk = *(vi->pk);

  /* Add link name to the map */
  size_t len = strlen(name);
  check(msgpack_pack_str(&pk, len));
  check(msgpack_pack_str_body(&pk, name, len));

  /* Now pack either the member object or a nil */
  if(vi->encode_members) {
    /* Now we need to pack the member object */
    switch(info->type) {
    case H5L_TYPE_HARD:
    case H5L_TYPE_EXTERNAL:
      /* If it's a hard or external link, pack the linked object . */
      check(pack_object_recursive(obj_id, name, pk, vi->depth+1, vi->max_depth, vi->data_size_limit, vi->buffer_size));
      break;
    case H5L_TYPE_SOFT:
      /* This is a soft link, so we'll just store the path to the object */
      check(pack_soft_link(obj_id, name, pk, info));
      break;
    case H5L_TYPE_ERROR:
      /* Something went wrong */
      goto cleanup;
    default:
      /* Unknown link type */
      check(pack_unknown(pk));
      break;
    }
  } else {
    /* We're not encoding the member objects, so just send a nil */
    check(msgpack_pack_nil(&pk));
  }

  /* Callback should return zero on success */
  result = 0;

 cleanup:
  return result;
}

/*
  Recursively pack a HDF5 group and its members to the supplied msgpack
  packer, subject to a maximum recursion depth. max_depth=0 means only
  object obj_id will be packed.

  For datasets smaller than data_size_limit we serialize the contents too.

  A group is represented as a msgpack map:

  {
  "hdf5_object" : "group",
  "attributes"  : (msgpack map with attributes, see pack_attributes.c)
  "members"     : (msgpack map with member groups and datasets, contains
                  {name : nil} for member groups we didn't recurse into)
  }

  Returns 0 on success, <0 on failure.
  Packer may contain partially written data on failure.
*/
int pack_group_recursive(hid_t obj_id, msgpack_packer pk, int depth,
                         int max_depth, size_t data_size_limit,
                         size_t buffer_size) {

  int result = -1;

  /* Check if we hit the recursion limit */
  if(depth > max_depth) {
    check(msgpack_pack_nil(&pk));
    return 0;
  }

  /* Determine number of members of this group */
  H5G_info_t ginfo;
  if(H5Gget_info(obj_id, &ginfo) < 0)goto cleanup;
  int nr_links = ginfo.nlinks;

  /*
    Recursion behavior:

    If depth=0 we encode metadata for all member groups and datasets.
    This includes dtype and shape for datasets, and the list of link
    names for groups.

    If depth > 0, we only encode member objects if there are not too
    many links and depth < max_depth.

    If depth == maxdepth == 0 then only groups will be packed as nil.
  */
  int encode_members = 1;
  if(depth > 0) {
    if((nr_links > MAX_LINKS_FOR_RECURSION) || (depth >= max_depth))encode_members = 0;
  }

  /* Make a msgpack map */
  check(msgpack_pack_map(&pk, 3));

  /* Add entry to identify this as a group */
  check(msgpack_pack_str(&pk, 11));
  check(msgpack_pack_str_body(&pk, "hdf5_object", 11));
  check(msgpack_pack_str(&pk, 5));
  check(msgpack_pack_str_body(&pk, "group", 5));

  /* Pack attributes of this group */
  check(msgpack_pack_str(&pk, 10));
  check(msgpack_pack_str_body(&pk, "attributes", 10));
  check(pack_attributes(obj_id, pk));

  /* Pack members of this group */
  check(msgpack_pack_str(&pk, 7));
  check(msgpack_pack_str_body(&pk, "members", 7));
  check(msgpack_pack_map(&pk, nr_links));

  /* Iterate over links and pack the member objects */
  hsize_t iter_index = 0;
  struct visit_info vi;
  vi.encode_members = encode_members;
  vi.depth = depth;
  vi.max_depth = max_depth;
  vi.data_size_limit = data_size_limit;
  vi.buffer_size = buffer_size;
  vi.pk = &pk;
  check(H5Literate2(obj_id, H5_INDEX_NAME, H5_ITER_NATIVE, &iter_index, visit_link, &vi));

  /* Success */
  result = 0;

 cleanup:
  return result;
}

/*
  Pack group starting at recursion depth zero.
*/
int pack_group(hid_t obj_id, msgpack_packer pk, int max_depth,
               size_t data_size_limit, size_t buffer_size) {
  return pack_group_recursive(obj_id, pk, 0, max_depth, data_size_limit, buffer_size);
}
