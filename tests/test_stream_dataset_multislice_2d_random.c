#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <hdf5.h>

#include "verify.h"
#include "pack_dataset.h"
#include "create_test_file.h"
#include "decode_ndarray.h"
#include "hdfstream.h"
#include "receive_ndarray.h"

static void stream_multiple_slices(struct hdfstream *hs, char *filename, char *datasetname,
                                   int rank, int nx, int ny, int nr_slices, hsize_t *start,
                                   hsize_t *count, bool succeeds) {
  (void) nx;
  verify(rank==2);

  /* Request and decode the specified slice(s) */
  size_t buffer_size = 100*1024;
  struct ndarray arr = receive_ndarray_slices(hs, filename, datasetname, nr_slices, rank, start, count, buffer_size);

  /* Check if the return code matches our expectation of whether this case should work */
  verify((arr.status==0) == succeeds);

  if(succeeds) {

    /* Compute expected size of result */
    hsize_t total_count[2] = {0, count[1]};
    if(nr_slices > 0) {
      for(int i=0; i<nr_slices; i+=1)
        total_count[0] += count[2*i+0];
    }

    /* Check array metadata */
    verify(arr.rank==2);
    verify(arr.shape[0] == total_count[0]);
    verify(arr.shape[1] == total_count[1]);

    /* Check array values */
    int *arr_data = (int *) arr.data;
    hsize_t row_nr = 0;
    /* Loop over requested slices */
    for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1) {
      /* Loop over elements in this slice */
      for(hsize_t i=0; i<count[2*slice_nr+0]; i+=1) {
        for(hsize_t j=0; j<count[2*slice_nr+1]; j+=1) {
        /* Get the value of this element from the decoded array */
          int unpacked_value = arr_data[j+total_count[1]*row_nr];
          /* Compute coordinates of this element in the full dataset */
          int input_i = i + start[2*slice_nr+0];
          int input_j = j + start[2*slice_nr+1];
          /* Compute the value we expect at these coordinates */
          int expected_value = input_j+10*ny*input_i;
          /* Check for agreement */
          verify(unpacked_value==expected_value);
        }
        row_nr += 1;
      }
    }
    if(arr.status==0)free(arr.data);
    printf("%d 2D slices decoded ok\n", nr_slices);
  } else {
    printf("%d 2D slices failed as expected\n", nr_slices);
  }
}


/*
  Check that we can correctly encode multiple slices of a 1D dataset to a
  single ndarray.

  In this case we're reading the dataset through the libhdfstream API
  instead of calling pack_dataset() directly.
*/
int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;
  
  printf("msgpack-c version is %s\n", MSGPACK_VERSION);

  /* Create a file for testing */
  char filename[] = "tmp/test_XXXXXX";
  hid_t file_id = create_temp_hdf5_file(filename);
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
  H5Dclose(dataset_id);
  H5Fclose(file_id);

  /* Initialize the process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(1, executable, 10, 10, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

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
    if(nr_slices > 0)stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, true);
  }

  /* Tidy up */
  hdfstream_free(hs);
  remove(filename);

  return 0;
}
