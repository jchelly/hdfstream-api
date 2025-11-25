#ifndef PACK_ATTRIBUTES_H
#define PACK_ATTRIBUTES_H

#include <hdf5.h>
#include <msgpack.h>

/*
  Pack attributes of a HDF5 object to the supplied msgpack packer.

  Returns 0 on success, <0 on failure.
*/
int pack_attributes(hid_t obj_id, msgpack_packer pk);

#endif
