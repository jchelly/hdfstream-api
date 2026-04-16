#include <msgpack.h>
#include <hdf5.h>

#include "decoder.h"
#include "pack_object.h"

/*
  Given a HDF5 object identifier, serialize it to an in-memory buffer
  and then decode it.
*/
hs_object pack_and_decode(hid_t obj_id, int max_depth,  size_t data_size_limit, size_t buffer_size);
