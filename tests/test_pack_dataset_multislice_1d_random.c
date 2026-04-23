#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <hdf5.h>

#include "verify.h"
#include "verify_all_closed.h"
#include "pack_dataset.h"
#include "create_test_file.h"
#include "decode_ndarray.h"
#include "pack_multiple_slices_1d.h"

/*
  Check that we can correctly encode multiple slices of a 1D dataset to a
  single ndarray.

  This runs the test with many random sequences of slices.
*/
int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  printf("msgpack-c version is %s\n", MSGPACK_VERSION);

  /* Create a file for testing */
  hid_t file_id = create_file_in_memory();
  verify(file_id>=0);

  /* Create a 1D int array */
  const int nr_elements = 10000;
  int *data = malloc(sizeof(int)*nr_elements);
  for(int i=0; i<nr_elements; i+=1)
    data[i] = i;

  /* Create a dataset and write the array to it */
  hsize_t dims[1] = {(hsize_t) nr_elements};
  hid_t dspace_id = H5Screate_simple(1, dims, NULL);
  hid_t dataset_id = H5Dcreate(file_id, "int_data_1d", H5T_NATIVE_INT,
                               dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  free(data);
  H5Sclose(dspace_id);

  /* RNG seed */
  unsigned int seed = 0;

  /* Arrays of slices */
  int max_slices_for_test = 10000;
  hsize_t start[max_slices_for_test];
  hsize_t count[max_slices_for_test];
  verify(max_slices_for_test <= HDFSTREAM_MAX_SLICES);

  /* Loop over test cases */
  int nr_reps = 100;
  for(int rep_nr=0; rep_nr<nr_reps; rep_nr+=1) {

    /* Decide maximum slice size for this iteration */
    hsize_t max_size = rand_r(&seed) % (nr_elements+1);

    /* Generate starts and counts for the slices */
    int nr_slices = 0;
    hsize_t offset = 0;
    while(true) {

      /* Possibly skip some elements */
      hsize_t nr_skipped = rand_r(&seed) % (max_size+1);

      /* Choose the size of this slice */
      hsize_t this_count = rand_r(&seed) % (max_size+1);

      if(offset + nr_skipped + this_count <= (hsize_t) nr_elements) {
        /* If the slice is in range, store it */
        offset += nr_skipped;
        start[nr_slices] = offset;
        count[nr_slices] = this_count;
        offset += this_count;
        nr_slices += 1;
      } else {
        /* Otherwise there's no room for any more slices */
        break;
      }
      if(nr_slices >= max_slices_for_test)break;
    }

    /* Run the test with the random slices */
    if(nr_slices > 0) {
      size_t buffer_sizes[] = {4, 8, 13, 32, 259, 1024, 10*1024};
      for(int i=0; i<7; i+=1) {
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, true, buffer_sizes[i], 0);
      }
    }
  }

  H5Dclose(dataset_id);
  H5Fclose(file_id);
  verify_all_closed();
  return 0;
}
