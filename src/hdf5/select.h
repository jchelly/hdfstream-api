#ifndef SELECT_H
#define SELECT_H

#include <hdf5.h>

herr_t select_slices(hid_t file_space_id, hsize_t nr_selections, hsize_t *select_start, hsize_t *select_count,
                     hsize_t *start, hsize_t *count);
#endif
