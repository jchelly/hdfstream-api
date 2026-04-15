#include <hdf5.h>
#include <msgpack.h>

#include "pack_object.h"
#include "pack_group.h"
#include "pack_dataset.h"
#include "pack_unknown.h"

int pack_object(hid_t loc_id, char *name, msgpack_packer pk, int max_depth,
                size_t data_size_limit, size_t buffer_size) {

  int result = -1;

  /* Open the specified HDF5 object */
  hid_t obj_id = H5Oopen(loc_id, name, H5P_DEFAULT);
  if(obj_id < 0)goto cleanup;

  /* Get the object type */
  H5I_type_t type = H5Iget_type(obj_id);

  /* Call the appropriate function to serialize this object */
  switch(type) {
  case H5I_GROUP:
    if(pack_group(obj_id, pk, max_depth, data_size_limit, buffer_size) < 0)goto cleanup;
    break;
  case H5I_DATASET:
    if(pack_dataset(obj_id, pk, data_size_limit, buffer_size) < 0)goto cleanup;
    break;
  default:
    if(pack_unknown(pk) < 0)goto cleanup;
    break;
  }

  /* Success */
  result = 0;

 cleanup:
  if(obj_id >= 0)H5Oclose(obj_id);
  return result;
}
