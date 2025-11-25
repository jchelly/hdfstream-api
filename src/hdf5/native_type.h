#ifndef NATIVE_TYPE_H
#define NATIVE_TYPE_H

#include <hdf5.h>

hid_t native_type(hid_t file_type_id);

hid_t make_packed_native_type(hid_t input_type_id);

#endif
