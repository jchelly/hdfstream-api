#include <limits.h>
#include <hdf5.h>
#include <msgpack.h>

#include "native_type.h"
#include "pack_dataset.h"
#include "pack_attributes.h"
#include "pack_numpy_type.h"
#include "pack_contents.h"
#include "multislice.h"
#include "verify.h"

#if !H5_VERSION_GE(1, 12, 0)
#error "HDF5 version 1.12.0 or newer is required."
#endif

/*
  Serialize multiple slices of a HDF5 dataset to the supplied msgpack packer.
  Data is read in chunks of up to buffer_size bytes. In this case the start
  and count arrays should have nr_slices*rank elements: the values for each
  slice are concatenated.

  In this implementation slices may only differ in the first dimension.
  Slices must be in ascending order of starting offset and must not overlap.
  They must be entirely within the dataspace in the file.

  If the dataset is scalar then the value is returned and nr_slices is
  ignored.

  Setting nr_slices=0 is not allowed because we would have no way to decide
  on the shape of the result.
*/
int pack_dataset_multi_slice_with_max_size(hid_t dataset_id, const int rank,
                                           const int nr_slices, const hsize_t *starts,
                                           const hsize_t *counts, const size_t buffer_size,
                                           msgpack_packer pk, const long long max_size) {

  int result = -1;
  hid_t file_type_id = -1;
  hid_t file_space_id = -1;
  hid_t mem_space_id = -1;
  hid_t mem_type_id = -1;
  char *buffer = NULL;
  int need_reclaim = 0;

  /* Don't allow zero slices */
  if(nr_slices < 1)goto cleanup;

  /* Get data type in the file */
  file_type_id = H5Dget_type(dataset_id);

  /* Get data type in memory, with any struct padding removed */
  mem_type_id = make_packed_native_type(file_type_id);
  if(mem_type_id < 0)goto cleanup;

  /* Get dataspace in the file */
  file_space_id = H5Dget_space(dataset_id);
  if(file_space_id < 0)goto cleanup;

  /* Get the size of the memory data type */
  size_t element_size = H5Tget_size(mem_type_id);

  /* Compute number of values per element in the first axis:
     Since we require all slices to be the same size in dimensions
     other then the first, we can just use the first slice here. */
  hsize_t nr_elements = 1;
  for(int i=1; i<rank; i+=1)
    nr_elements *= counts[i];

  /* Check if we have any vlen types */
  int have_vlen = detect_vlen_types(mem_type_id);
  if(have_vlen < 0)goto cleanup;

  /* Calculate how many elements we can fit in the read buffer */
  hsize_t elements_per_buffer = 0;
  if(nr_elements > 0) {
    elements_per_buffer = buffer_size / (nr_elements*element_size);
    if(elements_per_buffer < 1)goto cleanup; /* Can't buffer even one element, but we have >0 to encode */
  }

  /* Get dataset size in the file */
  hsize_t file_dims[HDFSTREAM_MAX_DIMS];
  if(H5Sget_simple_extent_dims(file_space_id, file_dims, NULL) < 0)goto cleanup;

  /* Initialize the slice object. This also bounds checks the slices */
  struct multislice ms;
  if(multislice_init(&ms, rank, nr_slices, starts, counts, elements_per_buffer, file_dims) != 0)goto cleanup;

  /* Allocate the read buffer */
  buffer = malloc(buffer_size);
  if(!buffer)goto cleanup;

  /* Get the number of elements in the first dimension */
  hsize_t array_size = (rank == 0) ? 1 : ms.total_count[0];

  /* Compute total size of the output array in bytes */
  size_t data_size = array_size*nr_elements*element_size;

  /* Encode the ndarray header */
  struct pack_contents_info pci;
  check(pack_contents_header(&pci, pk, max_size, mem_type_id, rank, ms.total_count, data_size));

  /* Allocate array for dimensions of each chunk to read */
  hsize_t chunk_dims[HDFSTREAM_MAX_DIMS];
  for(int i=0; i<rank; i+=1)
    chunk_dims[i] = ms.total_count[i];

  /* Read and encode array contents until done */
  while(true) {

    /* Try to select the next buffer full of elements to read */
    hsize_t nr_elements_to_read;
    int err = multislice_select_next_buffer_data(&ms, file_space_id, &nr_elements_to_read);

    /* Negative return value indicates failure */
    if(err < 0)goto cleanup;

    /* Positive return code indicates end of this array */
    if(err > 0)break;

    /* Set up the memory dataspace */
    if(rank > 0) {
      chunk_dims[0] = nr_elements_to_read;
      mem_space_id = H5Screate_simple(rank, chunk_dims, NULL);
    } else {
      mem_space_id = H5Screate(H5S_SCALAR);
    }

    /* Read the selected elements */
    if(H5Dread(dataset_id, mem_type_id, mem_space_id, file_space_id, H5P_DEFAULT, buffer) < 0)
      goto cleanup;
    if(have_vlen > 0)need_reclaim = 1;

    /* Pack the data to the msgpack_packer */
    hsize_t nr_bytes = nr_elements_to_read*nr_elements*element_size;
    check(pack_contents_body(&pci, pk, mem_type_id, nr_elements_to_read*nr_elements, buffer, nr_bytes));

    /* Free vlen data if necessary */
    if(need_reclaim) {
      H5Treclaim(mem_type_id, mem_space_id, H5P_DEFAULT, buffer);
      need_reclaim = 0;
    }

    /* Free the memory datasapace */
    H5Sclose(mem_space_id);
    mem_space_id = -1;
  }

  /* Success */
  result = 0;

 cleanup:

  if(need_reclaim) {
    H5Treclaim(mem_type_id, mem_space_id, H5P_DEFAULT, buffer);
    need_reclaim = 0;
  }
  if(buffer)free(buffer);
  if(file_space_id >= 0)H5Sclose(file_space_id);
  if(mem_space_id >=0)H5Sclose(mem_space_id);
  if(file_type_id >= 0)H5Tclose(file_type_id);
  if(mem_type_id >= 0)H5Tclose(mem_type_id);

  return result;
}

/*
  Pack a dataset to msgpack

  This includes the dataset's type, dimensions and attributes. The descriptor
  is packed as a msgpack map with the following entries:

  {
  "hdf5_object" : "dataset",
  "attributes"  : (msgpack map with attributes, see pack_attributes.c),
  "type"        : numpy type string,
  "shape"       : array with size in each dimension
  "data"        : numpy array with the data (only present if smaller than data_size_limit)
  }

  Returns <0 on failure, 0 on success.
*/
int pack_dataset(hid_t dataset_id, msgpack_packer pk, size_t data_size_limit, size_t buffer_size) {

  int result = -1;
  hid_t dspace_id = -1;
  hid_t file_type_id = -1;
  hid_t mem_type_id = -1;
  char *buffer = NULL;

  /* Get dataset dimensions */
  dspace_id = H5Dget_space(dataset_id);
  if(dspace_id < 0)goto cleanup;
  int rank = H5Sget_simple_extent_ndims(dspace_id);
  if(rank > HDFSTREAM_MAX_DIMS)goto cleanup;
  hsize_t dims[HDFSTREAM_MAX_DIMS];
  if(H5Sget_simple_extent_dims(dspace_id, dims, NULL) < 0)goto cleanup;

  /* Get dataset data type information */
  file_type_id = H5Dget_type(dataset_id);

  /* Get data type in memory, with any struct padding removed */
  mem_type_id = make_packed_native_type(file_type_id);
  if(mem_type_id < 0)goto cleanup;

  /* Determine whether to include dataset contents */
  size_t data_size = H5Tget_size(mem_type_id);
  if(data_size==0)goto cleanup;
  for(int i=0; i<rank; i+=1)
    data_size *= dims[i];
  int with_data = (data_size <= data_size_limit) ? 1 : 0;

  /* Make a msgpack map */
  if(with_data)
    check(msgpack_pack_map(&pk, 6));
  else
    check(msgpack_pack_map(&pk, 5));

  /* Add entry to identify this as a dataset */
  check(msgpack_pack_str(&pk, 11));
  check(msgpack_pack_str_body(&pk, "hdf5_object", 11));
  check(msgpack_pack_str(&pk, 7));
  check(msgpack_pack_str_body(&pk, "dataset", 7));

  /* Add entry with attributes */
  check(msgpack_pack_str(&pk, 10));
  check(msgpack_pack_str_body(&pk, "attributes", 10));
  check(pack_attributes(dataset_id, pk));

  /* Add type string */
  check(msgpack_pack_str(&pk, 4));
  check(msgpack_pack_str_body(&pk, "type", 4));
  check(pack_numpy_type(mem_type_id, NULL, &pk));

  /* Add kind string */
  check(msgpack_pack_str(&pk, 4));
  check(msgpack_pack_str_body(&pk, "kind", 4));
  check(pack_numpy_kind(mem_type_id, &pk));

  /* Add shape */
  check(msgpack_pack_str(&pk, 5));
  check(msgpack_pack_str_body(&pk, "shape", 5));
  check(msgpack_pack_array(&pk, rank));
  for(int i=0; i<rank; i+=1)
    check(msgpack_pack_unsigned_long_long(&pk, (unsigned long long) dims[i]));

  /* Pack dataset contents: this is a slice which includes all elements */
  if(with_data) {
    hsize_t start[HDFSTREAM_MAX_DIMS];
    hsize_t count[HDFSTREAM_MAX_DIMS];
    for(int i=0; i<rank;i+=1) {
      start[i] = 0;
      count[i] = dims[i];
    }
    check(msgpack_pack_str(&pk, 4));
    check(msgpack_pack_str_body(&pk, "data", 4));
    check(pack_dataset_slice(dataset_id, rank, start, count, buffer_size, pk));
  }

  /* Success */
  result = 0;

 cleanup:
  if(buffer)free(buffer);
  if(dspace_id >= 0)H5Sclose(dspace_id);
  if(file_type_id >= 0)H5Tclose(file_type_id);
  if(mem_type_id >= 0)H5Tclose(mem_type_id);
  return result;
}
