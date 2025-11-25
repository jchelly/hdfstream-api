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

  int rank = 2;

  /* Start with a valid selection - should work */
  {
    int nr_slices = 2;
    hsize_t start[] = {0,   0,   100, 0};
    hsize_t count[] = {100, 100, 200, 100};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, /* succeeds = */ true);
  }

  /* One of the slices is out of bounds in x */
  {
    int nr_slices = 2;
    hsize_t start[] = {0,   0,   9900, 0};
    hsize_t count[] = {100, 100, 200,  100};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, false);
  }

  /* One of the slices is out of bounds in y */
  {
    int nr_slices = 2;
    hsize_t start[] = {0,   0,   0,   9900};
    hsize_t count[] = {100, 100, 100, 200};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, false);
  }

  /* Both slices are out of bounds in y */
  {
    int nr_slices = 2;
    hsize_t start[] = {0,   9900, 0,   9900};
    hsize_t count[] = {100, 200,  100, 200};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, false);
  }

  /* Both slice are out of bounds in x */
  {
    int nr_slices = 2;
    hsize_t start[] = {9900,   0, 10100, 0};
    hsize_t count[] = {101,  100, 200,   100};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, false);
  }

  /* Slices have different offsets in y */
  {
    int nr_slices = 2;
    hsize_t start[] = {200,   0, 300, 100};
    hsize_t count[] = {100, 100, 100, 200};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, false);
  }

  /* Try three consecutive slices */
  {
    int nr_slices = 3;
    hsize_t start[] = {200, 100, 300, 100, 400, 100};
    hsize_t count[] = {100, 200, 100, 200, 100, 200};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, true);
  }

  /* Try three consecutive slices specified in the wrong order */
  {
    int nr_slices = 3;
    hsize_t start[] = {300, 100, 200, 100, 400, 100};
    hsize_t count[] = {100, 200, 100, 200, 100, 200};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, false);
  }

  /* Try three consecutive slices with zero size in y */
  {
    int nr_slices = 3;
    hsize_t start[] = {200, 100, 300, 100, 400, 100};
    hsize_t count[] = {100, 0,   100,   0, 100,   0};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, true);
  }

  /* Try three consecutive slices with zero size in x */
  {
    int nr_slices = 3;
    hsize_t start[] = {200, 100, 300, 100, 400, 100};
    hsize_t count[] = {  0, 200,   0, 200,   0, 200};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, true);
  }

  /* Try three consecutive slices with zero size in x and y */
  {
    int nr_slices = 3;
    hsize_t start[] = {200, 100, 300, 100, 400, 100};
    hsize_t count[] = {  0,   0,   0,   0,   0,   0};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, true);
  }

  /* Asking for zero slices should fail */
  {
    int nr_slices = 0;
    hsize_t start[] = {0};
    hsize_t count[] = {0};
    stream_multiple_slices(hs, filename, "int_data_2d", rank, nx, ny, nr_slices, start, count, false);
  }

  /* Tidy up */
  hdfstream_free(hs);
  remove(filename);

  return 0;
}
