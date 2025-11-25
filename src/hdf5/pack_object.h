#ifndef PACK_OBJECT_H
#define PACK_OBJECT_H

#include <hdf5.h>
#include <msgpack.h>

/*
  Recursively pack a HDF5 object and its members to the supplied msgpack
  packer, subject to a maximum recursion depth. max_depth=0 means only
  object obj_id will be packed.

  This identifies the obejct by name and calls pack_group() or
  pack_dataset() as appropriate.
*/

int pack_object(hid_t file_id, char *name, msgpack_packer pk, int max_depth,
                size_t data_size_limit, size_t buffer_size);

#endif
