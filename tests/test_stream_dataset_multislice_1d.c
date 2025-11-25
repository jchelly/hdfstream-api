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
                                   int rank, int nr_slices, hsize_t *start, hsize_t *count, bool succeeds) {

  /* Request and decode the specified slice(s) */
  size_t buffer_size = 100*1024;
  struct ndarray arr = receive_ndarray_slices(hs, filename, datasetname, nr_slices, rank, start, count, buffer_size);

  /* Check if the return code matches our expectation of whether this case should work */
  verify((arr.status==0) == succeeds);

  if(succeeds) {

    /* Compute expected size of result */
    hsize_t total_count = 0;
    if(nr_slices > 0) {
      for(int i=0; i<nr_slices; i+=1)
        total_count += count[i];
    }

    /* Check array metadata */
    verify(arr.rank==1);
    verify(arr.shape[0] == total_count);

    /* Check array values */
    hsize_t row_nr = 0;
    for(int slice_nr=0; slice_nr<nr_slices; slice_nr+=1) {
      for(hsize_t i=0; i<count[slice_nr]; i+=1) {
        int *arr_data = (int *) arr.data;
        verify(arr_data[row_nr] == ((int) (start[slice_nr]+i)));
        row_nr += 1;
      }
    }
    if(arr.status==0)free(arr.data);
    printf("%d 1D slices decoded ok\n", nr_slices);
  } else {
    printf("%d 1D slices failed as expected\n", nr_slices);
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
  H5Dclose(dataset_id);
  H5Fclose(file_id);

  /* Initialize the process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(1, executable, 10, 10, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

  {
    /* Try one zero sized slice */
    int nr_slices = 1;
    hsize_t start[] = {284};
    hsize_t count[] = {0};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, true);
  }

  {
    /* Try one small slice */
    int nr_slices = 1;
    hsize_t start[] = {9230};
    hsize_t count[] = {37};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, true);
  }

  {
    /* Try two small slices */
    int nr_slices = 2;
    hsize_t start[] = {0, 100};
    hsize_t count[] = {50, 20};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, true);
  }

  {
    /* Read all of the elements in one big slice */
    int nr_slices = 1;
    hsize_t start[] = {0};
    hsize_t count[] = {10000};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, true);
  }

  {
    /* Read all of the elements in a few slices */
    int nr_slices = 5;
    hsize_t start[] = {0,    2000, 4000, 6000, 8000};
    hsize_t count[] = {2000, 2000, 2000, 2000, 2000};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, true);
  }

  {
    /* Throw in some zero sized slices */
    int nr_slices = 8;
    hsize_t start[] = {0,    2000, 4000, 4000, 6000, 6000, 8000, 10000};
    hsize_t count[] = {2000, 2000,    0, 2000,    0, 2000, 2000, 0};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, true);
  }

  {
    /* Try zero slices */
    int nr_slices = 0;
    hsize_t start[] = {0};
    hsize_t count[] = {0};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, false);
  }

  {
    /* Try an out of bounds slice */
    int nr_slices = 1;
    hsize_t start[] = {nr_elements};
    hsize_t count[] = {1};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, false);
  }

  {
    /* Try another out of bounds slice */
    int nr_slices = 1;
    hsize_t start[] = {0};
    hsize_t count[] = {nr_elements+1};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, false);
  }

  {
    /* One slice of three out of bounds */
    int nr_slices = 3;
    hsize_t start[] = {0,  5000, 10000};
    hsize_t count[] = {10, 10,   10};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, false);
  }

  {
    /* Slices which are not in ascending order */
    int nr_slices = 2;
    hsize_t start[] = {800, 300};
    hsize_t count[] = {100, 100};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, false);
  }

  {
    /* Slices which overlap */
    int nr_slices = 2;
    hsize_t start[] = {500, 100};
    hsize_t count[] = {510, 100};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, false);
  }

  {
    /* Multiple invalid slices */
    int nr_slices = 5;
    hsize_t start[] = {0,   500, 300, 350, 9000};
    hsize_t count[] = {500, 100, 100, 100, 1500};
    stream_multiple_slices(hs, filename, "int_data_1d", /* rank= */ 1, nr_slices, start, count, false);
  }

  /* Tidy up */
  hdfstream_free(hs);
  remove(filename);

  return 0;
}
