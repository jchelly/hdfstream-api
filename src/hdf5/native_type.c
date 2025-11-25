#include <hdf5.h>

#include "native_type.h"
#include "slice_limits.h"

/*
  Given a data type in the file, return type to use in memory.
  Returns a new HDF5 data type object.
*/
hid_t native_type(hid_t file_type_id) {

  hid_t mem_type_id = H5Tget_native_type(file_type_id, H5T_DIR_DEFAULT);

  H5T_class_t class = H5Tget_class(mem_type_id);
  switch(class) {
  case H5T_STRING:
    /* Ensure strings are null padded to avoid serializing uninitialized memory */
    H5Tset_strpad(mem_type_id, H5T_STR_NULLPAD);
    break;
  default:
    break;
  }
  return mem_type_id;
}


/*
  Create a native datatype with padding removed for serialization.

  Returns a new data type id (which must be H5Tclosed) on success,
  H5I_INVALID_HID otherwise.
*/
hid_t make_packed_native_type(hid_t input_type_id) {

  if(input_type_id == H5I_INVALID_HID)return H5I_INVALID_HID;

  H5T_class_t class = H5Tget_class(input_type_id);
  switch(class) {
  case H5T_INTEGER:
  case H5T_ENUM:
  case H5T_FLOAT:
  case H5T_STRING:
    {
      /* For atomic types, find the corresponding native type  */
      return native_type(input_type_id);
    }
  case H5T_ARRAY:
    {
      /* This is an array type. Get its dimensions. */
      int rank = H5Tget_array_ndims(input_type_id);
      hsize_t dims[HDFSTREAM_MAX_DIMS];
      H5Tget_array_dims(input_type_id, dims);

      /* Get the type of the array elements */
      hid_t element_type_id = H5Tget_super(input_type_id);

      /* Make a packed version of the element type */
      hid_t packed_element_type_id = make_packed_native_type(element_type_id);
      H5Tclose(element_type_id);

      /* Create the new array type */
      hid_t output_type_id = H5Tarray_create(packed_element_type_id, rank, dims);
      H5Tclose(packed_element_type_id);
      return output_type_id;
    }
  case H5T_COMPOUND:
    {
      /* This is a compound type, so make a packed copy */
      hid_t output_type_id = H5Tcopy(input_type_id);
      H5Tpack(output_type_id);
      return output_type_id;
    }
  case H5T_VLEN:
    {
      /* For vlens, we need to tight pack the type of the elements */
      hid_t element_type_id = H5Tget_super(input_type_id);
      hid_t packed_element_type_id = make_packed_native_type(element_type_id);
      H5Tclose(element_type_id);
      hid_t output_type_id = H5Tvlen_create(packed_element_type_id);
      H5Tclose(packed_element_type_id);
      return output_type_id;
    }
  default:
    {
      /* We don't know how to deal with other types */
      return H5I_INVALID_HID;
    }
  }
}
