#include <stdlib.h>

#include <hdf5.h>
#include <msgpack.h>

#include "pack_group.h"
#include "pack_dataset.h"
#include "pack_object.h"
#include "pack_attributes.h"
#include "pack_soft_link.h"
#include "type_mapping.h"
#include "verify.h"

#if !H5_VERSION_GE(1, 12, 0)
#error "HDF5 version 1.12.0 or newer is required."
#endif

/* If a group contains too many links, just return the link names */
#define MAX_LINKS_FOR_RECURSION 20

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
int pack_group(hid_t obj_id, msgpack_packer pk, int max_depth,
               size_t data_size_limit, size_t buffer_size) {

  int result = -1;
  H5O_info2_t *oinfo = NULL;
  H5L_info2_t *linfo = NULL;
  char *name = NULL;

  /* Check if we hit the recursion limit */
  if(max_depth < 0) {
    check(msgpack_pack_nil(&pk));
    return 0;
  }

  /* Determine number of members of this group */
  H5G_info_t ginfo;
  if(H5Gget_info(obj_id, &ginfo) < 0)goto cleanup;
  int nr_links = ginfo.nlinks;

  /* Determine whether we try to encode member objects */
  int encode_members = 1;
  if((nr_links > MAX_LINKS_FOR_RECURSION) || (max_depth <= 0))encode_members = 0;

  /* Get metadata for group members */
  oinfo = malloc(sizeof(H5O_info2_t)*nr_links);
  if(!oinfo)goto cleanup;
  linfo = malloc(sizeof(H5L_info2_t)*nr_links);
  if(!linfo)goto cleanup;
  for(int link_nr=0; link_nr<nr_links; link_nr+=1) {
    /* Get the link type */
    if(H5Lget_info_by_idx2(obj_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, link_nr, &linfo[link_nr], H5P_DEFAULT) < 0)goto cleanup;
    switch(linfo[link_nr].type) {
    case H5L_TYPE_HARD:
    case H5L_TYPE_EXTERNAL:
      if(encode_members) {
        /* Get object info for hard or external links */
        if(H5Oget_info_by_idx3(obj_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, link_nr, &oinfo[link_nr], H5O_INFO_BASIC, H5P_DEFAULT) < 0)goto cleanup;
      }
      break;
    case H5L_TYPE_SOFT:
      /* We serialize soft link paths without dereferencing them */
      break;
    case H5L_TYPE_ERROR:
      /* Something went wrong */
      goto cleanup;
    default:
      /* Nothing to do for unknown link types */
      break;
    }
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
  for(int link_nr=0; link_nr<nr_links; link_nr+=1) {
    /* Get name of this group member */
    ssize_t len = H5Lget_name_by_idx(obj_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, link_nr, NULL, 0, H5P_DEFAULT);
    if(len < 0)goto cleanup;
    name = malloc(len+1);
    len = H5Lget_name_by_idx(obj_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, link_nr, name, len+1, H5P_DEFAULT);
    if(len < 0)goto cleanup;
    /* Add name to the map */
    check(msgpack_pack_str(&pk, len));
    check(msgpack_pack_str_body(&pk, name, len));
    /* Now pack either the member object or a nil */
    if(encode_members) {
      /* Now we need to pack the member object */
      switch(linfo[link_nr].type) {
      case H5L_TYPE_HARD:
      case H5L_TYPE_EXTERNAL:
        /* If it's a hard or external link to a group or dataset, pack it. */
        if((oinfo[link_nr].type == H5O_TYPE_GROUP) || (oinfo[link_nr].type == H5O_TYPE_DATASET)) {
          check(pack_object(obj_id, name, pk, max_depth-1, data_size_limit, buffer_size));
        } else {
          /* Link to an unknown object type */
          check(msgpack_pack_nil(&pk));
        }
        break;
      case H5L_TYPE_SOFT:
        /* This is a soft link, so we'll just store the path to the object */
        check(pack_soft_link(obj_id, name, pk, &linfo[link_nr]));
        break;
      case H5L_TYPE_ERROR:
        /* Something went wrong */
        goto cleanup;
      default:
        /* Unknown link type */
        check(msgpack_pack_nil(&pk));
        break;
      }
      free(name);
      name = NULL;
    } else {
      /* We're not encoding the member objects, so just send a nil */
      check(msgpack_pack_nil(&pk));
    }
  }

  /* Success */
  result = 0;

 cleanup:
  if(oinfo)free(oinfo);
  if(linfo)free(linfo);
  if(name)free(name);
  return result;
}
