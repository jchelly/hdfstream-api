#ifndef MULTISLICE_H
#define MULTISLICE_H

#include <hdf5.h>
#include "slice_limits.h"

struct multislice {
  int rank;
  int nr_slices;
  const hsize_t *starts;
  const hsize_t *counts;
  hsize_t elements_per_buffer;
  hsize_t elements_left_total;
  int slice_nr;
  hsize_t offset_in_slice;
  /* Total size of the output array(s) */
  hsize_t total_count[HDFSTREAM_MAX_DIMS];
  /* Start and count for selecting hyperslabs */
  hsize_t start[HDFSTREAM_MAX_DIMS];
  hsize_t count[HDFSTREAM_MAX_DIMS];
};

/*
   Initialize a multislice struct. Returns -1 if input params are invalid, 0 otherwise

   ms : pointer to the struct
   rank : number of dimensions of the dataset
   nr_slices : number of slices to read
   starts : array of length rank*nr_slices with offsets to the start of each slice
   counts : array of length rank*nr_slices with lengths of each slice
   elements_per_buffer : number of elements in the first dimension we can fit in the buffer
   elements_per_array : number of elements in the first dimension we can encode as one array
*/
int multislice_init(struct multislice *ms, const int rank, const int nr_slices,
                    const hsize_t *starts, const hsize_t *counts,
                    const size_t elements_per_buffer, const hsize_t *file_dims);

/*
  Select the data for the next read operation using H5Sselect_hyperslab().

  Return codes:

   0 : success
   1 : no more data to read for this array
  -1 : failure
*/
int multislice_select_next_buffer_data(struct multislice *ms, hid_t file_space_id, hsize_t *nr_elements_to_read);

#endif
