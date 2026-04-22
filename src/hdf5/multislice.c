#include <hdf5.h>
#include <assert.h>
#include <stdlib.h>
#include "slice_limits.h"
#include "multislice.h"
#include "select.h"

static hsize_t min_size(const hsize_t a, const hsize_t b) {
  if(a < b)
    return a;
  else
    return b;
}

int multislice_init(struct multislice *ms, const int rank, int nr_slices,
                    const hsize_t *starts, const hsize_t *counts,
                    const size_t elements_per_buffer, const hsize_t *file_dims) {

  /* Store parameters */
  ms->rank = rank;
  ms->nr_slices = (rank > 0) ? nr_slices : 0;
  ms->starts = starts;
  ms->counts = counts;
  ms->elements_per_buffer = elements_per_buffer;

  /* Sanity check rank and number of slices */
  if(rank > HDFSTREAM_MAX_DIMS)return -1;
  if(rank < 0)return -1;
  if(nr_slices > HDFSTREAM_MAX_SLICES)return -1;
  if(nr_slices < 1)return -1;

  /* Bounds check the slices */
  for(int slice_nr=0; slice_nr<ms->nr_slices; slice_nr+=1) {

    /* Find start and count arrays for this slice */
    const hsize_t *start1 = starts + rank*slice_nr;
    const hsize_t *count1 = counts + rank*slice_nr;

    /* Check that this slice is within the dataspace on disk */
    for(int dim_nr=0; dim_nr<rank; dim_nr+=1) {
      if(start1[dim_nr] + count1[dim_nr] > file_dims[dim_nr])return -1;
    }

    if(slice_nr > 0) {
      /* Find start and count arrays for the previous slice */
      const hsize_t *start0 = starts + ((hsize_t) rank)*(slice_nr-1);
      const hsize_t *count0 = counts + ((hsize_t) rank)*(slice_nr-1);

      /* Slices must be in ascending order of offset in the first dimension */
      if(start1[0] < start0[0])return -1;

      /* Slices must not overlap */
      if(start0[0]+count0[0] > start1[0])return -1;

      /* Start and count in all dimensions but the first should be equal */
      for(int dim_nr=1; dim_nr<rank; dim_nr+=1) {
        if(start0[dim_nr] != start1[dim_nr])return -1;
        if(count0[dim_nr] != count1[dim_nr])return -1;
      }
    }
  }

  /* Compute total size of the output array(s). All slices should have the
     same size in the first dimension, so we can use the first slice only. */
  for(int i=1; i<rank; i+=1)
    ms->total_count[i] = counts[i];

  /* Set start and count parameters for hyperslabs. First dimension
     varies between read operations and will be filled in later. */
  for(int i=1; i<rank; i+=1) {
    ms->start[i] = starts[i];
    ms->count[i] = counts[i];
  }

  /* Slices will be concatenated in the first dimension */
  if(rank > 0) {
    ms->total_count[0] = 0;
    for(int i=0; i<ms->nr_slices; i+=1)
      ms->total_count[0] += counts[i*rank+0];
  }

  /* Set current slice index and offset to the start of the first slice */
  ms->slice_nr = 0;
  ms->offset_in_slice = 0;
  if(rank > 0)
    ms->elements_left_total = ms->total_count[0];
  else
    ms->elements_left_total = 1; /* scalar quantity */

  /* If the size is zero in any dimension, there's nothing to read */
  for(int i=0; i<rank; i+=1) {
    if(ms->total_count[i] == 0)ms->elements_left_total = 0;
  }

  return 0;
}

/*
  Select the data for the next read operation using H5Sselect_hyperslab().
*/
int multislice_select_next_buffer_data(struct multislice *ms, hid_t file_space_id, hsize_t *nr_elements_to_read) {

  hid_t select_space_id = -1;
  hsize_t *select_start = NULL;
  hsize_t *select_count = NULL;
  int result = -1; /* Indicates failure */

  /* Check if we hit the end */
  if(ms->elements_left_total == 0) {
    result = 1;
    goto cleanup;
  }

  if(ms->rank == 0) {
    /* In case of scalars, just select all and return */
    H5Sselect_all(file_space_id);
    ms->elements_left_total = 0; /* There's nothing more to select */
    assert(ms->nr_slices==0);       /* Scalars cannot be sliced */
    *nr_elements_to_read = 1;       /* Scalars always have one data element */
    result = 0;
    goto cleanup;
  } else {
    /* In case of arrays, clear the selection */
    H5Sselect_none(file_space_id);
  }

  /*
    Will need to accumulate selection info

    Calling H5Sselect_hyperslab for N slices has O(N^2) runtime,
    so instead we gather all slice information and recursively
    combine selections.
  */
  hsize_t nr_selections = 0;
  select_start = malloc(sizeof(hsize_t)*ms->elements_per_buffer);
  select_count = malloc(sizeof(hsize_t)*ms->elements_per_buffer);

  /* Compute how many elements we can select: one buffer full, or until end (whichever is less) */
  *nr_elements_to_read = 0;
  hsize_t elements_left_in_buffer = min_size(ms->elements_per_buffer, ms->elements_left_total);
  while(elements_left_in_buffer > 0) {

    /* Find the start and count for the current slice */
    assert(ms->slice_nr < ms->nr_slices);
    hsize_t slice_start = ms->starts[ms->slice_nr*ms->rank];
    hsize_t slice_count = ms->counts[ms->slice_nr*ms->rank];

    /* Compute how many elements we can read from this slice */
    hsize_t elements_left_in_slice = slice_count - ms->offset_in_slice;
    hsize_t elements_to_read = min_size(elements_left_in_slice, elements_left_in_buffer);

    /* Store this selection, if it contains any elements */
    if(elements_to_read > 0) {
      assert(nr_selections < ms->elements_per_buffer);
      select_start[nr_selections] = slice_start + ms->offset_in_slice;
      select_count[nr_selections] = elements_to_read;
      nr_selections += 1;
    }

    /* Advance to the next part of the slice */
    ms->offset_in_slice += elements_to_read;

    /* May also need to advance to the next slice */
    if(ms->offset_in_slice >= slice_count) {
      ms->offset_in_slice = 0;
      ms->slice_nr += 1;
      assert(ms->slice_nr <= ms->nr_slices);
    }

    /* Update remaining space in the buffer etc */
    assert(elements_left_in_buffer >= elements_to_read);
    elements_left_in_buffer -= elements_to_read;
    assert(ms->elements_left_total >= elements_to_read);
    ms->elements_left_total  -= elements_to_read;

    /* Update count of elements to be read in the first dimension */
    *nr_elements_to_read += elements_to_read;
  }

  /* Select all of the slices  */
  if(select_slices(file_space_id, nr_selections, select_start, select_count, ms->start, ms->count) < 0) {
    result = -1;
    goto cleanup;
  }

  /* Success */
  result = 0;

 cleanup:
  if(select_space_id >= 0)H5Sclose(select_space_id);
  if(select_start)free(select_start);
  if(select_count)free(select_count);
  return result;
}
