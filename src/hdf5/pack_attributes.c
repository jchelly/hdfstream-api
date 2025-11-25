#include "pack_attributes.h"

#include <stdlib.h>
#include <hdf5.h>
#include <msgpack.h>

#include "native_type.h"
#include "pack_dataset.h"
#include "pack_contents.h"
#include "pack_numpy_type.h"
#include "verify.h"

#if !H5_VERSION_GE(1, 12, 0)
#error "HDF5 version 1.12.0 or newer is required."
#endif

/*
  Pack attributes of a HDF5 object to the supplied msgpack packer.

  Returns 0 on success, <0 on failure.
  Packer may contain partially written data on failure.
*/
int pack_attributes(hid_t obj_id, msgpack_packer pk) {

  int result = -1;
  hid_t attr_id = -1;
  hid_t dspace_id = -1;
  hid_t file_type_id = -1;
  hid_t mem_type_id = -1;
  char *buffer = NULL;
  char *name = NULL;
  int need_reclaim = 0;

  /* Determine how many attributes this object has */
  H5O_info2_t oinfo;
  herr_t err = H5Oget_info3(obj_id, &oinfo,  H5O_INFO_NUM_ATTRS);
  if(err < 0)goto cleanup;
  int nr_attrs = oinfo.num_attrs;

  /* Add a map to contain the attribute names and values */
  check(msgpack_pack_map(&pk, nr_attrs));

  /* Loop over attributes */
  for(int attr_nr=0; attr_nr<nr_attrs; attr_nr+=1) {

    /* Open this attribute */
    attr_id = H5Aopen_by_idx(obj_id, ".", H5_INDEX_NAME, H5_ITER_INC,
                             (hsize_t) attr_nr, H5P_DEFAULT, H5P_DEFAULT);
    if(attr_id < 0)goto cleanup;

    /* Get the name of this attribute */
    ssize_t len = H5Aget_name(attr_id, 0, NULL);
    if(len < 0)goto cleanup;
    name = malloc(len+1);
    len = H5Aget_name(attr_id, len+1, name);

    /* Add the attribute name to the map */
    check(msgpack_pack_str(&pk, len));
    check(msgpack_pack_str_body(&pk, name, len));

    /* Get attribute dimensions */
    dspace_id = H5Aget_space(attr_id);
    if(dspace_id < 0)goto cleanup;
    int rank = H5Sget_simple_extent_ndims(dspace_id);
    if(rank > HDFSTREAM_MAX_DIMS)goto cleanup;
    hsize_t dims[HDFSTREAM_MAX_DIMS];
    if(H5Sget_simple_extent_dims(dspace_id, dims, NULL) < 0)goto cleanup;

    /* Get attribute data type information */
    file_type_id = H5Aget_type(attr_id);

    /* Get data type in memory, with any struct padding removed */
    mem_type_id = make_packed_native_type(file_type_id);
    if(mem_type_id < 0)goto cleanup;

    /* Check if we read any vlen data which will need to be freed */
    int have_vlen = detect_vlen_types(mem_type_id);
    if(have_vlen < 0)goto cleanup;

    /* Read attribute data into memory */
    size_t count = 1;
    for(int i=0; i<rank; i+=1)
      count *= dims[i];
    size_t nr_bytes = count*H5Tget_size(mem_type_id);
    buffer = malloc(nr_bytes);
    if(H5Aread(attr_id, mem_type_id, buffer) < 0)goto cleanup;

    /* Check if we read any vlen data which will need to be freed */
    if(have_vlen > 0)need_reclaim = 1;

    /* Pack the body of the attribute */
    struct pack_contents_info pci;
    const size_t max_size = ((1LL << 32)-1LL);
    check(pack_contents_header(&pci, pk, max_size, mem_type_id, rank, dims, nr_bytes));
    check(pack_contents_body(&pci, pk, mem_type_id, count, buffer, nr_bytes));

    /* Tidy up before we process the next attribute */
    if(need_reclaim) {
      H5Treclaim(mem_type_id, dspace_id, H5P_DEFAULT, buffer);
      need_reclaim = 0;
    }
    free(name);
    name = NULL;
    H5Aclose(attr_id);
    attr_id = -1;
    H5Sclose(dspace_id);
    dspace_id = -1;
    H5Tclose(file_type_id);
    file_type_id = -1;
    H5Tclose(mem_type_id);
    mem_type_id = -1;
    free(buffer);
    buffer = NULL;
  }

  /* Success */
  result = 0;

 cleanup:
  if(need_reclaim) {
    H5Treclaim(mem_type_id, dspace_id, H5P_DEFAULT, buffer);
    need_reclaim = 0;
  }
  if(name)free(name);
  if(attr_id >= 0)H5Aclose(attr_id);
  if(dspace_id >= 0)H5Sclose(dspace_id);
  if(file_type_id >=0)H5Tclose(file_type_id);
  if(mem_type_id >= 0)H5Tclose(mem_type_id);
  if(buffer)free(buffer);
  return result;
}
