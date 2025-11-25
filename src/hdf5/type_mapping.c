#include "type_mapping.h"

#include <stdio.h>
#include <hdf5.h>

/*
  Given a HDF5 data type in memory, return a numpy style type descriptor.
  Returns 0 on success, non-zero otherwise.

  Does not work on compound or vlen types.
*/
int numpy_type_info(const hid_t dtype_id, const size_t len, char *descr) {

  H5T_class_t class = H5Tget_class(dtype_id);
  char kind;

  /*
    Determine the numpy kind identifier of this data type
  */
  switch(class) {
  case H5T_INTEGER:
  case H5T_ENUM:
    {
      /* The dataset in the file is an integer type */
      H5T_sign_t sign = H5Tget_sign(dtype_id);
      switch(sign) {
      case H5T_SGN_NONE:
	/* Unsigned integer type */
	kind='u';
	break;
      case H5T_SGN_2:
	/* Two's complement signed integer type */
	kind='i';
	break;
      default:
	/* Don't know how to handle this type */
	goto error;
      }
    }
    break;
  case H5T_FLOAT:
    {
      /* The dataset in the file is a float type */
      kind='f';
    }
    break;
  case H5T_STRING:
    {
      /* Check for vlen string */
      htri_t is_vlen = H5Tis_variable_str(dtype_id);
      if(is_vlen > 0) {
	/* Can't handle vlen strings yet */
	goto error;
      }
      /* It's a fixed size string datatype */
      kind='S';
      break;
    }
  default:
    /* Don't know how to handle this type */
    goto error;
  }

  /*
    Now need to create the numpy type string
  */
  switch(class) {
  case H5T_INTEGER:
  case H5T_FLOAT:
  case H5T_ENUM:
    {
      /* Determine endianness of the memory data type */
      char endian = (char) 0;
      H5T_order_t order = H5Tget_order(dtype_id);
      switch(order) {
      case H5T_ORDER_LE:
	endian='<';
	break;
      case H5T_ORDER_BE:
	endian='>';
	break;
      default:
	/* Don't know how to handle this type */
	goto error;
      }
      /* Get size of type in memory */
      size_t size = H5Tget_size(dtype_id);
      /* Generate the type description */
      snprintf(descr, len, "%c%c%d", endian, kind, (int) size);
    }
    break;
  case H5T_STRING:
    {
      /* Get size of type in memory */
      size_t str_size = H5Tget_size(dtype_id);
      /* Generate the type description */
      snprintf(descr, len, "S%d", (int) str_size);
    }
    break;
  default:
    /* Don't know how to handle this type */
    goto error;
  }
  return 0;

 error:
  descr[0] = (char) 0;
  return -1;
}
