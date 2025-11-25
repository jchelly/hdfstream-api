#include <hdf5.h>
#include <msgpack.h>

#include "verify.h"
#include "pack_numpy_type.h"
#include "pack_contents.h"


/*
  Compute how many msgpack_bins of size max_size are needed to store nr_bytes.
  Always returns at least 1.
*/
static size_t nr_bins_needed(size_t max_size, size_t nr_bytes) {

  assert(max_size > 0);
  size_t nr_bins = (nr_bytes + max_size - 1) / max_size;
  assert(nr_bins*max_size >= nr_bytes);
  return nr_bins;
}


int pack_contents_header(struct pack_contents_info *pci, msgpack_packer pk, size_t max_size,
                         hid_t dtype_id, const int rank, const hsize_t dims[], const size_t nr_bytes) {

  /* Store max object size for use when packing body and any recursive vlen elements */
  pci->max_size = max_size;

  /* Determine if the data contains any vlen types */
  pci->has_vlen = detect_vlen_types(dtype_id);
  if(pci->has_vlen > 0) {
    /*
      This type contains vlen or variable string components. In this case we
      return a msgpack array with one entry for each element in the flattened
      array.
    */
    size_t nr_elements = 1;
    for(int i=0; i<rank; i+=1)
      nr_elements *= dims[i];
    /* vlen arrays larger than max_size are not supported for now */
    if(nr_elements > max_size)return -1;
    /* Add attribute data to the map in the format recognised by msgpack-numpy */
    check(msgpack_pack_map(&pk, 3));
    /* Send vlen:True */
    check(msgpack_pack_str(&pk, 4));
    check(msgpack_pack_str_body(&pk, "vlen", 4));
    check(msgpack_pack_true(&pk));
    /* Send shape (array with one element per dimension) */
    check(msgpack_pack_str(&pk, 5));
    check(msgpack_pack_str_body(&pk, "shape", 5));
    check(msgpack_pack_array(&pk, rank));
    for(int i=0; i<rank; i+=1)
      check(msgpack_pack_unsigned_long_long(&pk, (unsigned long long) dims[i]));
    /* Send msgpack array header */
    check(msgpack_pack_str(&pk, 4));
    check(msgpack_pack_str_body(&pk, "data", 4));
    check(msgpack_pack_array(&pk, nr_elements));
    return 0;

  } else if(pci->has_vlen==0) {

    /*
      This is a fixed size data type, so we return a binary blob containing an ndarray
      Determine how many msgpack_bin obejcts we need to use for the data.
    */
    pci->nr_bins = nr_bins_needed(max_size, nr_bytes);
    /* Add attribute data to the map in the format recognised by msgpack-numpy */
    check(msgpack_pack_map(&pk, 6));
    /* Send nd:True */
    check(msgpack_pack_str(&pk, 2));
    check(msgpack_pack_str_body(&pk, "nd", 2));
    check(msgpack_pack_true(&pk));
    /* Send type */
    check(msgpack_pack_str(&pk, 4));
    check(msgpack_pack_str_body(&pk, "type", 4));
    check(pack_numpy_type(dtype_id, NULL, &pk));
    /* Send kind (empty string for scalars, "V" for array or compound) */
    check(msgpack_pack_str(&pk, 4));
    check(msgpack_pack_str_body(&pk, "kind", 4));
    check(pack_numpy_kind(dtype_id, &pk));
    /* Send shape (array with one element per dimension) */
    check(msgpack_pack_str(&pk, 5));
    check(msgpack_pack_str_body(&pk, "shape", 5));
    check(msgpack_pack_array(&pk, rank));
    for(int i=0; i<rank; i+=1)
      check(msgpack_pack_unsigned_long_long(&pk, (unsigned long long) dims[i]));
    /* Send total number of bytes in the array (useful for zero copy decoding) */
    check(msgpack_pack_str(&pk, 6));
    check(msgpack_pack_str_body(&pk, "nbytes", 6));
    check(msgpack_pack_long_long(&pk, (long long) nr_bytes));
    /* Send data field, which will contain an array of zero or more binary objects  */
    check(msgpack_pack_str(&pk, 4));
    check(msgpack_pack_str_body(&pk, "data", 4));
    check(msgpack_pack_array(&pk, pci->nr_bins));
    /* Initialize counters used for packing contents */
    pci->total_bytes_left = nr_bytes;
    pci->bytes_left_in_bin = 0; /* Zero indicates msgpack_bin header not written yet */
    /* Success! */
    return 0;

  } else {

    /* vlen type detect failed somehow */
    return -1;

  }
 cleanup:
  /* We get here if a check() call fails */
  return -1;
}

int pack_contents_body(struct pack_contents_info *pci, msgpack_packer pk,
                       hid_t dtype_id, const size_t count, void *buffer, const size_t buffer_len) {

  hid_t vl_type_id = -1;
  if(pci->has_vlen > 0) {

    /*
      This type contains vlen or variable string components. What we do here
      depends on the data type class.
    */
    H5T_class_t class = H5Tget_class(dtype_id);
    switch(class) {
    case H5T_INTEGER:
    case H5T_ENUM:
    case H5T_FLOAT:
      /* Should not get here if dtype_id is a fixed size type */
      assert(pci->has_vlen==0);
      return -1;
    case H5T_ARRAY:
    case H5T_COMPOUND:
      /* vlen components inside these types are not implemented yet */
      return -1;
    case H5T_VLEN:
      /* In this case each element is a variable length array, represented
         by an instance of struct hvl_t in the buffer. We need to recursively
         pack these arrays. */
      vl_type_id = H5Tget_super(dtype_id);
      size_t vl_size = H5Tget_size(vl_type_id);
      hvl_t *hvl = (hvl_t *) buffer;
      for(size_t i=0; i<count; i+=1) {
        hsize_t vl_count = hvl[i].len;
        hsize_t vl_dims[1] = {vl_count};
        struct pack_contents_info element_pci;
        check(pack_contents_header(&element_pci, pk, pci->max_size, vl_type_id, 1, vl_dims, vl_count*vl_size));
        check(pack_contents_body(&element_pci, pk, vl_type_id, vl_count, hvl[i].p, vl_count*vl_size));
      }
      H5Tclose(vl_type_id);
      return 0;
    case H5T_STRING:
      /* In this case dtype_id should be a variable length string */
      assert(H5Tis_variable_str(dtype_id) > 0);
      /* The buffer contains an array of char * string pointers, which we're
         going to serialize as msgpack string objects */
      char **str = (char **) buffer;
      for(size_t i=0; i<count; i+=1) {
        size_t len = strlen(str[i]);
        check(msgpack_pack_str(&pk, len));
        check(msgpack_pack_str_body(&pk, str[i], len));
      }
      return 0;
    default:
      /* Can't handle this type */
      return -1;
    }
  } else if(pci->has_vlen==0) {

    /*
      Fixed size data types are written out as one or more msgpack_bin objects
    */
    size_t buffer_len_left = buffer_len;
    size_t buffer_offset = 0;
    while(buffer_len_left > 0) {

      /* Write a new msgpack_bin header if necessary */
      if(pci->bytes_left_in_bin == 0) {
        assert(pci->nr_bins > 0);
        size_t bin_size = (pci->total_bytes_left < pci->max_size) ? pci->total_bytes_left : pci->max_size;
        pci->bytes_left_in_bin = bin_size;
        pci->total_bytes_left -= bin_size; /* Number of bytes in remaining bin objects after this one */
        check(msgpack_pack_bin(&pk, bin_size));
      }

      /* Compute how many bytes we can write to the current msgpack_bin on this iteration */
      size_t bytes_to_write = (buffer_len_left < pci->bytes_left_in_bin) ? buffer_len_left : pci->bytes_left_in_bin;

      /* Write the data */
      assert(buffer_offset+bytes_to_write <= buffer_len);
      check(pk.callback(pk.data, ((char *) buffer)+buffer_offset, bytes_to_write));

      /* Update bytes left to write on this call and position in input buffer */
      assert(buffer_len_left >= bytes_to_write);
      buffer_len_left -= bytes_to_write;
      buffer_offset += bytes_to_write;

      /* Update bytes remaining to be written to the current msgpack_bin */
      assert(pci->bytes_left_in_bin >= bytes_to_write);
      pci->bytes_left_in_bin -= bytes_to_write;
    }
    return 0;

  } else {

    /* vlen type detect failed somehow */
    return -1;

  }
 cleanup:
  /* We get here if a check() call fails */
  if(vl_type_id >= 0)H5Tclose(vl_type_id);
  return -1;
}
