#ifndef PACK_CONTENTS_H
#define PACK_CONTENTS_H

#include <hdf5.h>
#include <msgpack.h>

/*
  Information that needs to be stored between pack_contents_header and
  pack_contents_body calls
*/
struct pack_contents_info {
  /* Msgpack object size limit */
  size_t max_size;
  /* Whether we have any vlen data types (0=fixed length, 1=vlen present) */
  int has_vlen;
  /* For fixed size types */
  size_t nr_bins;
  size_t total_bytes_left;
  size_t bytes_left_in_bin;
  /* For vlen types */
  int nr_arrays;
  size_t total_nr_elements;
  size_t nr_elements_written;
};

/*
  Generate the msgpack header for the body of a dataset or attribute

  pk       - msgpack packer object
  dtype_id - HDF5 data type of the dataset or attribute
  rank     - number of dimensions of the data
  dims     - size in each dimension
  nr_bytes - total size of the H5Dread buffer required

  Returns 0 on success, non-zero otherwise.
*/
int pack_contents_header(struct pack_contents_info *pci, msgpack_packer pk, size_t max_size,
                         hid_t dtype_id, const int rank, const hsize_t dims[], const size_t nr_bytes);

/*
  Generate msgpack serialization of (possibly part of) the body of a dataset or attribute

  pk       - msgpack packer object
  dtype_id - HDF5 data type of the dataset or attribute
  count    - number of elements to serialize
  buffer   - buffer with the element data
  nr_bytes - size of the H5Dread buffer for this part of the body

  Returns 0 on success, non-zero otherwise.
*/
int pack_contents_body(struct pack_contents_info *pci, msgpack_packer pk,
                       hid_t dtype_id, const size_t count, void *buffer, const size_t nr_bytes);
#endif
