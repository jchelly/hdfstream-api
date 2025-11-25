#ifndef PACK_SOFT_LINK_H
#define PACK_SOFT_LINK_H

#include <hdf5.h>
#include <msgpack.h>


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

int pack_soft_link(hid_t obj_id, const char *name, msgpack_packer pk, H5L_info2_t *link_info);

#endif
