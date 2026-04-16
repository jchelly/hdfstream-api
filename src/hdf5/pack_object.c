#include <hdf5.h>
#include <msgpack.h>

#include "pack_object.h"
#include "pack_group.h"
#include "pack_dataset.h"
#include "pack_unknown.h"
#include "verify.h"

int pack_object_recursive(hid_t loc_id, const char *name, msgpack_packer pk, int depth,
                          int max_depth, size_t data_size_limit, size_t buffer_size) {

  int result = -1;
  hid_t obj_id = -1;

  /* Open the specified HDF5 object */
  obj_id = H5Oopen(loc_id, name, H5P_DEFAULT);
  if(obj_id < 0)goto cleanup;

  /* Get the object type */
  H5I_type_t type = H5Iget_type(obj_id);

  /* Call the appropriate function to serialize this object */
  switch(type) {
  case H5I_GROUP:
    /* Don't encode the group if we hit the recursion limit */
    if(depth > max_depth)
      check(msgpack_pack_nil(&pk));
    else
      check(pack_group_recursive(obj_id, pk, depth, max_depth, data_size_limit, buffer_size));
    break;
  case H5I_DATASET:
    check(pack_dataset(obj_id, pk, data_size_limit, buffer_size));
    break;
  default:
    check(pack_unknown(pk));
    break;
  }

  /* Success */
  result = 0;

 cleanup:
  if(obj_id >= 0)H5Oclose(obj_id);
  return result;
}


int pack_object(hid_t loc_id, const char *name, msgpack_packer pk, int max_depth,
                size_t data_size_limit, size_t buffer_size) {
  return pack_object_recursive(loc_id, name, pk, /* depth = */ 0, max_depth, data_size_limit, buffer_size);
}
