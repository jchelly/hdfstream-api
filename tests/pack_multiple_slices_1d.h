#include <hdf5.h>
#include <stdbool.h>

#ifndef PACK_MULTIPLE_SLICES_1D_H
#define PACK_MULTIPLE_SLICES_1D_H

void pack_multiple_slices_1d(hid_t dataset_id, int rank, int nr_slices,
			     hsize_t *start, hsize_t *count, bool succeeds,
			     size_t buffer_size, size_t max_size);
#endif
