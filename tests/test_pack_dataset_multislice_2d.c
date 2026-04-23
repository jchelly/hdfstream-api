#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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
  Check that encoding invalid slices of a 2D dataset fails as expected
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

  int rank = 2;

  /* Will repeat the test with several buffer sizes */
  size_t buffer_sizes[] = {4, 8, 13, 32, 259, 1024, 10*1024};
  for(int buffer_size_nr=0; buffer_size_nr<7; buffer_size_nr+=1) {
    size_t buffer_size = buffer_sizes[buffer_size_nr];

    /* Will also try several different maximum array sizes (in bytes) */
    size_t array_sizes[] = {0, 4, 5, 10, 16, 30};
    for(int array_size_nr=0; array_size_nr<6; array_size_nr+=1) {
      size_t max_size = array_sizes[array_size_nr];

      /* Start with a valid selection - should work */
      {
        int nr_slices = 2;
        hsize_t start[] = {0,   0,   100, 0};
        hsize_t count[] = {100, 100, 200, 100};
        bool succeeds = will_succeed(nr_slices, count, buffer_size, max_size);
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, succeeds, buffer_size, max_size);
      }

      /* One of the slices is out of bounds in x */
      {
        int nr_slices = 2;
        hsize_t start[] = {0,   0,   9900, 0};
        hsize_t count[] = {100, 100, 200,  100};
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, false, buffer_size, max_size);
      }

      /* One of the slices is out of bounds in y */
      {
        int nr_slices = 2;
        hsize_t start[] = {0,   0,   0,   9900};
        hsize_t count[] = {100, 100, 100, 200};
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, false, buffer_size, max_size);
      }

      /* Both slices are out of bounds in y */
      {
        int nr_slices = 2;
        hsize_t start[] = {0,   9900, 0,   9900};
        hsize_t count[] = {100, 200,  100, 200};
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, false, buffer_size, max_size);
      }

      /* Both slice are out of bounds in x */
      {
        int nr_slices = 2;
        hsize_t start[] = {9900,   0, 10100, 0};
        hsize_t count[] = {101,  100, 200,   100};
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, false, buffer_size, max_size);
      }

      /* Slices have different offsets in y */
      {
        int nr_slices = 2;
        hsize_t start[] = {200,   0, 300, 100};
        hsize_t count[] = {100, 100, 100, 200};
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, false, buffer_size, max_size);
      }

      /* Try three consecutive slices */
      {
        int nr_slices = 3;
        hsize_t start[] = {200, 100, 300, 100, 400, 100};
        hsize_t count[] = {100, 200, 100, 200, 100, 200};
        bool succeeds = will_succeed(nr_slices, count, buffer_size, max_size);
        /* Valid slice, should work */
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, succeeds, buffer_size, max_size);
        /* Repeat with buffer slightly too small to store one row (200 ints) */
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, false, 199*sizeof(int), max_size);
        /* Then try again with the buffer just big enough */
        succeeds = true; /* ((max_size >= 200*sizeof(int)) || (max_size == 0)); */
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, succeeds, 200*sizeof(int), max_size);
      }

      /* Try three consecutive slices specified in the wrong order */
      {
        int nr_slices = 3;
        hsize_t start[] = {300, 100, 200, 100, 400, 100};
        hsize_t count[] = {100, 200, 100, 200, 100, 200};
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, false, buffer_size, max_size);
      }

      /* Try three consecutive slices with zero size in y - should work regardless of buffer and array size limits */
      {
        int nr_slices = 3;
        hsize_t start[] = {200, 100, 300, 100, 400, 100};
        hsize_t count[] = {100, 0,   100,   0, 100,   0};
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, true, buffer_size, max_size);
      }

      /* Try three consecutive slices with zero size in x */
      {
        int nr_slices = 3;
        hsize_t start[] = {200, 100, 300, 100, 400, 100};
        hsize_t count[] = {  0, 200,   0, 200,   0, 200};
        bool succeeds = will_succeed(nr_slices, count, buffer_size, max_size);
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, succeeds, buffer_size, max_size);
      }

      /* Try three consecutive slices with zero size in x and y */
      {
        int nr_slices = 3;
        hsize_t start[] = {200, 100, 300, 100, 400, 100};
        hsize_t count[] = {  0,   0,   0,   0,   0,   0};
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, true, buffer_size, max_size);
      }

      /* Asking for zero slices should fail */
      {
        int nr_slices = 0;
        hsize_t start[] = {0};
        hsize_t count[] = {0};
        pack_multiple_slices_2d(dataset_id, rank, nx, ny, nr_slices, start, count, false, buffer_size, max_size);
      }

    }
  }

  H5Dclose(dataset_id);
  H5Fclose(file_id);
  verify_all_closed();
  return 0;
}
