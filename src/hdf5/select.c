#include "select.h"

herr_t select_slices(hid_t file_space_id, hsize_t nr_selections,
                     hsize_t *select_start, hsize_t *select_count,
                     hsize_t *start, hsize_t *count) {

  for(hsize_t selection_nr=0; selection_nr<nr_selections; selection_nr+=1) {
    start[0] = select_start[selection_nr];
    count[0] = select_count[selection_nr];
    herr_t err = H5Sselect_hyperslab(file_space_id, H5S_SELECT_OR, start, NULL, count, NULL);
    if(err < 0)return err;
  }
  return 0;
}
