#include "select.h"

herr_t select_slices_direct(hid_t file_space_id, hsize_t nr_selections,
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


static hid_t select_slices_recursive(hid_t file_space_id,
                                     hsize_t first_selection, hsize_t nr_selections,
                                     hsize_t *select_start, hsize_t *select_count,
                                     hsize_t *start, hsize_t *count) {

  hid_t space_id_left  = -1;
  hid_t space_id_right = -1;
  hid_t result = -1;
  if(nr_selections == 0) {

    /* No selections, so select nothing */
    result = H5Scopy(file_space_id);
    if(result < 0)goto cleanup;
    if(H5Sselect_none(result) >= 0)return result;

  } else if(nr_selections == 1) {

    /* One selection, so select it */
    result = H5Scopy(file_space_id);
    if(result < 0)goto cleanup;
    start[0] = select_start[first_selection];
    count[0] = select_count[first_selection];
    if(H5Sselect_hyperslab(result, H5S_SELECT_SET, start, NULL, count, NULL) >= 0)return result;

  } else {

    /* Multiple selections, so we'll split them and do the two halves recursively */
    hsize_t nr_left = nr_selections / 2;
    hsize_t first_on_left = first_selection;
    hsize_t nr_right = nr_selections - nr_left;
    hsize_t first_on_right = first_selection + nr_left;
    space_id_left  = select_slices_recursive(file_space_id, first_on_left,  nr_left, select_start, select_count, start, count);
    if(space_id_left < 0)goto cleanup;
    space_id_right = select_slices_recursive(file_space_id, first_on_right, nr_right, select_start, select_count, start, count);
    if(space_id_right < 0)goto cleanup;
    result = H5Scombine_select(space_id_left, H5S_SELECT_OR, space_id_right);
    if(result < 0)goto cleanup;
    H5Sclose(space_id_left);
    H5Sclose(space_id_right);
    return result;
  }

 cleanup:
  if(result >= 0)H5Sclose(result);
  if(space_id_left >= 0)H5Sclose(space_id_left);
  if(space_id_right >= 0)H5Sclose(space_id_right);
  return -1;
}


herr_t select_slices(hid_t file_space_id, hsize_t nr_selections,
                     hsize_t *select_start, hsize_t *select_count,
                     hsize_t *start, hsize_t *count) {

  hid_t dspace_id = select_slices_recursive(file_space_id, 0, nr_selections,
                                            select_start, select_count, start, count);
  if(dspace_id < 0)return -1;
  herr_t err = H5Sselect_copy(file_space_id, dspace_id);
  H5Sclose(dspace_id);
  return err;
}
