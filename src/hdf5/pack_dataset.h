#ifndef PACK_DATASET_H
#define PACK_DATASET_H

#include <hdf5.h>
#include <msgpack.h>

#include "slice_limits.h"

/*
  Serialize a single slice of a HDF5 dataset to the supplied msgpack packer.
  Data is read in chunks of up to buffer_size bytes.

  This should produce a msgpack map which msgpack-numpy can interpret
  as a numpy.ndarray.
*/
#define pack_dataset_slice(dataset_id, rank, start, count, buffer_size, pk) \
  pack_dataset_multi_slice_with_max_size(dataset_id, rank, 1, start, count, buffer_size, pk, ((1LL << 32)-1LL))

/*
  Serialize multiple dataset slices to a single encoded ndarray using the msgpack
  default maximum array or binary object size of 2^32-1.
*/
#define pack_dataset_multi_slice(dataset_id, rank, nr_slices, starts, counts, buffer_size, pk) \
  pack_dataset_multi_slice_with_max_size(dataset_id, rank, nr_slices, starts, counts, buffer_size, pk, ((1LL << 32)-1LL))

/*
  Serialize multiple dataset slices to a single encoded ndarray using a
  specified maximum array size. Setting a small max_size is useful for testing.

  If the result doesn't fit in the specified size then multiple arrays should
  be returned.
*/
int pack_dataset_multi_slice_with_max_size(hid_t dataset_id, const int rank,
                                           const int nr_slices, const hsize_t *starts,
                                           const hsize_t *counts, const size_t buffer_size,
                                           msgpack_packer pk, long long max_size);
/*
  Pack a dataset to msgpack

  This includes the dataset's type, dimensions and attributes. The descriptor
  is packed as a msgpack map with the following entries:

  {
  "hdf5_object" : "dataset",
  "attributes"  : (msgpack map with attributes, see pack_attributes.c),
  "type"        : numpy type string,
  "shape"       : array with size in each dimension
  "data"        : numpy array with the data (only present smaller than
                  data_size_limit)
  }

  Returns <0 on failure, 0 on success.
*/
int pack_dataset(hid_t dataset_id, msgpack_packer pk, size_t data_size_limit, size_t buffer_size);

#endif
