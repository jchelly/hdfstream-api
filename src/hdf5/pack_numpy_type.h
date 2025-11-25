#ifndef PACK_NUMPY_TYPE_H
#define PACK_NUMPY_TYPE_H

#include <hdf5.h>
#include <msgpack.h>

int pack_numpy_type(hid_t dtype_id, char *field_name, msgpack_packer *pk);
int pack_numpy_kind(hid_t dtype_id, msgpack_packer *pk);

int detect_vlen_types(hid_t dtype_id);

#endif
