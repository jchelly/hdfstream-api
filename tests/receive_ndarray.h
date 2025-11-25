#ifndef RECEIVE_NDARRAY_H
#define RECEIVE_NDARRAY_H

#include <hdf5.h>
#include "hdfstream.h"
#include "decode_ndarray.h"

/* Request and decode one or more slices of a dataset */
struct ndarray receive_ndarray_slices(struct hdfstream *hs, char *filename, char *datasetname,
                                      int nr_slices, int rank, hsize_t *start,
                                      hsize_t *count, const size_t buffer_size);

/* Request and decode one slice of a dataset */
#define receive_ndarray(hs, filename, datasetname, rank, start, count, buffer_size) \
  receive_ndarray_slices(hs, filename, datasetname, 1, rank, start, count, buffer_size)

#endif
