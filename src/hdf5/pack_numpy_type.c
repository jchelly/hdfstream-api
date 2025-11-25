#include "pack_numpy_type.h"

#include <hdf5.h>
#include <msgpack.h>
#include "type_mapping.h"

#define HDFSTREAM_MAX_DIMS 32

/*
  Pack the supplied HDF5 type to a msgpack packer in the form
  recognised by the msgpack-numpy python module.

  For atomic datatypes the result is a single string (e.g. "<i4"
  for 4 byte little endian ints).

  For compound datatypes we generate nested arrays:

  ((field1_name, field1_type[, field1_size]),
   (field2_name, field2_type[, field2_size]),
   ...)

   where the field type entries are generated recursively in the
   case of nested compound types.

   HDF5 data type classes supported:

     H5T_INTEGER
     H5T_FLOAT
     H5T_STRING (but not vlen strings)
     H5T_ARRAY
     H5T_COMPOUND

   HDF5 data type classes not implemented yet:

     H5T_TIME
     H5T_BITFIELD
     H5T_OPAQUE
     H5T_REFERENCE
     H5T_VLEN

  Currently H5T_ENUMs are treated as integers.

*/
int pack_numpy_type(hid_t dtype_id, char *field_name, msgpack_packer *pk) {

  const int descr_len = 10;
  char descr[descr_len];

  H5T_class_t class = H5Tget_class(dtype_id);
  switch(class) {
  case H5T_INTEGER:
  case H5T_ENUM:
  case H5T_FLOAT:
  case H5T_STRING:
    {
      /* Special case for vlen strings: just return string "str" as the type */
      if((class == H5T_STRING) && (H5Tis_variable_str(dtype_id) > 0)) {
	msgpack_pack_str(pk, 3);
	msgpack_pack_str_body(pk, "str", 3);
	return 0;
      }

      /* This is a scalar type. Get the type string. */
      if(numpy_type_info(dtype_id, descr_len, descr) != 0)return -1;
      if(field_name) {
	/* This is a scalar field in a compound type. Pack (field_name, type_string). */
	msgpack_pack_array(pk, 2);
	msgpack_pack_str(pk, strlen(field_name));
	msgpack_pack_str_body(pk, field_name, strlen(field_name));
	msgpack_pack_str(pk, strlen(descr));
	msgpack_pack_str_body(pk, descr, strlen(descr));
	return 0;
      } else {
	/* If we have no field name we can just pack the type string. */
	msgpack_pack_str(pk, strlen(descr));
	msgpack_pack_str_body(pk, descr, strlen(descr));
	return 0;
      }
    }
    break;

  case H5T_ARRAY:
    {
      /* Array type. Will return ([field_name], type, dimensions). First pack field name, if any. */
      if(field_name) {
	msgpack_pack_array(pk, 3);
	msgpack_pack_str(pk, strlen(field_name));
	msgpack_pack_str_body(pk, field_name, strlen(field_name));
      }
      else {
	msgpack_pack_array(pk, 2);
      }
      /* Pack the data type of the array elements */
      hid_t element_type = H5Tget_super(dtype_id);
      int res = pack_numpy_type(element_type, field_name, pk);
      H5Tclose(element_type);
      if(res < 0)return -1;

      /* Then pack the dimensions of the array */
      hsize_t dims[HDFSTREAM_MAX_DIMS];
      int rank = H5Tget_array_ndims(dtype_id);
      H5Tget_array_dims(dtype_id, dims);
      msgpack_pack_array(pk, rank);
      for(int i=0; i<rank; i+=1)
	msgpack_pack_long(pk, (long) dims[i]);
      return 0;
    }
    break;

  case H5T_COMPOUND:
    {
      /* This is a compound type. Will need to pack all of the fields. */
      int nr_fields = H5Tget_nmembers(dtype_id);
      if(nr_fields < 0)return -1;
      msgpack_pack_array(pk, nr_fields);

      /* Loop over fields and pack */
      for(int field_nr=0; field_nr<nr_fields; field_nr+=1) {
	hid_t field_type_id = H5Tget_member_type(dtype_id, field_nr);
	char *sub_field_name = H5Tget_member_name(dtype_id, field_nr);
	int res = pack_numpy_type(field_type_id, sub_field_name, pk);
	H5Tclose(field_type_id);
	H5free_memory(sub_field_name);
	if(res != 0)return -1;
      }
      return 0;
    }
    break;

  case H5T_VLEN:
    {
      /* For vlen types we return the underlying type */
      hid_t vl_type_id = H5Tget_super(dtype_id);
      int result = pack_numpy_type(vl_type_id, NULL, pk);
      H5Tclose(vl_type_id);
      return result;
    }
    break;

  default:
    /* Can't handle this type */
    return -1;
  }

  /* Should never get here */
  return -1;
}

/*
  Pack the numpy kind code for the supplied type
*/
int pack_numpy_kind(hid_t dtype_id, msgpack_packer *pk) {

  H5T_class_t class = H5Tget_class(dtype_id);
  switch(class) {
  case H5T_INTEGER:
  case H5T_ENUM:
  case H5T_FLOAT:
  case H5T_STRING:
    /* Scalar, so kind='' */
    msgpack_pack_str(pk, 0);
    msgpack_pack_str_body(pk, NULL, 0);
    return 0;
    break;
  case H5T_ARRAY:
  case H5T_COMPOUND:
  case H5T_VLEN:
    /* These have kind="V" */
    msgpack_pack_str(pk, 1);
    msgpack_pack_str_body(pk, "V", 1);
    return 0;
    break;
  default:
    /* Can't handle this type */
    return -1;
  }
  return -1;
}

/*
  Determine if the supplied data type contains variable length strings or
  vlen types, which will need special treatment when encoding the body of
  the dataset or attribute. Return value:

  -1: failure
   0: no variable length types present
   1: found at least one variable length type

   This is used to determine if datasets or attributes read with H5[AD]read()
   will contain pointers that need to be expanded to inline data when we
   serialize the contents.

*/
int detect_vlen_types(hid_t dtype_id) {

  H5T_class_t class = H5Tget_class(dtype_id);
  switch(class) {
  case H5T_STRING:
    {
      /* Strings may or may not be variable length */
      if(H5Tis_variable_str(dtype_id) > 0)
	return 1;
      else
	return 0;
    }
    break;
  case H5T_VLEN:
    {
      /* This is a variable length type */
      return 1;
    }
    break;
  case H5T_ARRAY:
    {
      /* This is an array. Need to check the type of the elements. */
      hid_t element_type = H5Tget_super(dtype_id);
      int is_vlen = detect_vlen_types(element_type);
      H5Tclose(element_type);
      return is_vlen;
    }
    break;
  case H5T_COMPOUND:
    {
      /* This is a struct. Need to check for any vlen fields. */
      int nr_fields = H5Tget_nmembers(dtype_id);
      if(nr_fields < 0)return -1;
      for(int field_nr=0; field_nr<nr_fields; field_nr+=1) {
	hid_t field_type_id = H5Tget_member_type(dtype_id, field_nr);
	int is_vlen = detect_vlen_types(field_type_id);
	H5Tclose(field_type_id);
	if(is_vlen != 0)return is_vlen;
      }
      return 0;
    }
    break;
  case H5T_INTEGER:
  case H5T_ENUM:
  case H5T_FLOAT:
    {
      /* These are all fixed size */
      return 0;
    }
    break;
  default:
    /* Unknown type class */
    return -1;
  }
}
