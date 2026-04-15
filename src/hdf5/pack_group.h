#ifndef PACK_GROUP_H
#define PACK_GROUP_H

#include <hdf5.h>
#include <msgpack.h>


/*
  Recursively pack a HDF5 group and its members to the supplied msgpack
  packer, subject to a maximum recursion depth. max_depth=0 means only
  object obj_id will be packed.

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
                         size_t buffer_size);

int pack_group(hid_t obj_id, msgpack_packer pk, int max_depth,
               size_t data_size_limit, size_t buffer_size);
#endif
