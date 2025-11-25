#ifndef TYPE_MAPPING_H
#define TYPE_MAPPING_H

#include <stddef.h>
#include <hdf5.h>

int numpy_type_info(const hid_t dtype_id, const size_t len, char *descr);

#endif
