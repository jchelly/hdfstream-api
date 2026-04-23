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
#include "pack_multiple_slices_2d.h"


/*
  Check if a set of slices can be read with a given buffer size and array size limit

  The buffer needs to be able to hold one "row", i.e. all data for one index
  in the first dimension. This same amount of data must be able to fit in one
  encoded ndarray too.
*/
static bool will_succeed(int nr_slices, hsize_t *count, size_t buffer_size, size_t max_size) {

  (void) nr_slices;

  /* Handle default max size specified by max_size=0 */
  if(max_size==0)max_size = (1LL << 32) - 1LL;

  /* Assume data elements are ints */
  const size_t element_size = sizeof(int);

  /* Find total size of result in the second dimension */
  size_t nr_elements = count[1]; /* Slices are concatenated in first dimension */

  /* Find number of bytes for each element in the first dimension */
  size_t min_bytes = element_size*nr_elements;

  /* return ((max_size >= min_bytes) && (buffer_size >= min_bytes)); */

  /* max_size is not currently used */
  return (buffer_size >= min_bytes);
}


/*
  Check that we can correctly encode multiple slices of a 2D dataset to a
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

  /* Create a 2D int array */
  const int nx = 500;
  const int ny = 500;
  int *data = malloc(sizeof(int)*nx*nx);
  for(int i=0; i<nx; i+=1) {
    for(int j=0; j<ny; j+=1) {
      data[j+ny*i] = j+10*ny*i;
    }
  }

  /* Create a dataset and write the array to it */
  hsize_t dims[2] = {(hsize_t) nx, (hsize_t) ny};
  hid_t dspace_id = H5Screate_simple(2, dims, NULL);
  hid_t dataset_id = H5Dcreate(file_id, "int_data_2d", H5T_NATIVE_INT,
                               dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  free(data);
  H5Sclose(dspace_id);

  /* RNG seed */
  unsigned int seed = 0;

  /* Arrays of slices */
  int max_slices_for_test = 1000;
  int rank = 2;
  hsize_t start[rank*max_slices_for_test];
  hsize_t count[rank*max_slices_for_test];
  verify(max_slices_for_test <= HDFSTREAM_MAX_SLICES);

  /* Loop over test cases */
  int nr_reps = 1000;
  for(int rep_nr=0; rep_nr<nr_reps; rep_nr+=1) {

    /* Decide maximum slice size in x for this iteration */
    hsize_t max_size = rand_r(&seed) % (nx+1);

    /* Choose slice in the y direction */
    hsize_t count_y = rand_r(&seed) % (ny+1);
    hsize_t start_y = rand_r(&seed) % (ny - ((int) count_y) + 1);

    /* Generate starts and counts for the slices */
    int nr_slices = 0;
    hsize_t offset = 0;
    while(true) {

      /* Possibly skip some elements */
      hsize_t nr_skipped = rand_r(&seed) % (max_size + 1);

      /* Choose the size of this slice */
      hsize_t this_count = rand_r(&seed) % (max_size + 1);

      if(offset + nr_skipped + this_count <= (hsize_t) nx) {
        /* If the slice is in range, store it */
        offset += nr_skipped;
        start[2*nr_slices+0] = offset;
        start[2*nr_slices+1] = start_y;
        count[2*nr_slices+0] = this_count;
        count[2*nr_slices+1] = count_y;
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

      /* Will repeat the test with several buffer sizes */
      size_t buffer_sizes[] = {4, 8, 13, 32, 259, 1024, 3001, 10*1024};
      for(int buffer_size_nr=0; buffer_size_nr<8; buffer_size_nr+=1) {
        size_t buffer_size = buffer_sizes[buffer_size_nr];

        /* Will also try several different maximum array sizes (in bytes, zero means the default 4GB) */
        size_t array_sizes[] = {0, 30, 73, 398};
        for(int array_size_nr=0; array_size_nr<4; array_size_nr+=1) {
          size_t this_max_size = array_sizes[array_size_nr];
          bool succeeds = will_succeed(nr_slices, count, buffer_size, this_max_size);
          pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, succeeds, buffer_size, this_max_size);
        }
      }
    }
  }
  H5Dclose(dataset_id);
  H5Fclose(file_id);
  verify_all_closed();
  return 0;
}
