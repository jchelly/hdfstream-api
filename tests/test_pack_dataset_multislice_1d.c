#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <hdf5.h>
#include <msgpack.h>

#include "verify.h"
#include "create_test_file.h"
#include "pack_multiple_slices_1d.h"

/*
  Check that we can correctly encode multiple slices of a 1D dataset to a
  single ndarray
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

  /* Will repeat the test with several buffer sizes */
  size_t buffer_sizes[] = {4, 8, 13, 32, 259, 1024, 10*1024};
  for(int buffer_size_nr=0; buffer_size_nr<7; buffer_size_nr+=1) {
    size_t buffer_size = buffer_sizes[buffer_size_nr];

    /* Will also try several different maximum array sizes (in bytes) */
    size_t array_sizes[] = {0, 4, 5, 10, 16, 30};
    for(int array_size_nr=0; array_size_nr<6; array_size_nr+=1) {
      size_t max_size = array_sizes[array_size_nr];

      {
	/* Try one zero sized slice */
	int nr_slices = 1;
	hsize_t start[] = {284};
	hsize_t count[] = {0};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, true, buffer_size, max_size);
      }

      {
	/* Try one small slice */
	int nr_slices = 1;
	hsize_t start[] = {9230};
	hsize_t count[] = {37};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, true, buffer_size, max_size);
      }

      {
	/* Try two small slices */
	int nr_slices = 2;
	hsize_t start[] = {0, 100};
	hsize_t count[] = {50, 20};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, true, buffer_size, max_size);
      }

      {
	/* Read all of the elements in one big slice */
	int nr_slices = 1;
	hsize_t start[] = {0};
	hsize_t count[] = {10000};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, true, buffer_size, max_size);
      }

      {
	/* Read all of the elements in a few slices */
	int nr_slices = 5;
	hsize_t start[] = {0,    2000, 4000, 6000, 8000};
	hsize_t count[] = {2000, 2000, 2000, 2000, 2000};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, true, buffer_size, max_size);
      }

      {
	/* Throw in some zero sized slices */
	int nr_slices = 8;
	hsize_t start[] = {0,    2000, 4000, 4000, 6000, 6000, 8000, 10000};
	hsize_t count[] = {2000, 2000,    0, 2000,    0, 2000, 2000, 0};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, true, buffer_size, max_size);
      }

      {
	/* Try zero slices */
	int nr_slices = 0;
	hsize_t start[1] = {0};
	hsize_t count[1] = {0};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, false, buffer_size, max_size);
      }

      {
	/* Try an out of bounds slice */
	int nr_slices = 1;
	hsize_t start[] = {nr_elements};
	hsize_t count[] = {1};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, false, buffer_size, max_size);
      }

      {
	/* Try another out of bounds slice */
	int nr_slices = 1;
	hsize_t start[] = {0};
	hsize_t count[] = {nr_elements+1};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, false, buffer_size, max_size);
      }

      {
	/* One slice of three out of bounds */
	int nr_slices = 3;
	hsize_t start[] = {0,  5000, 10000};
	hsize_t count[] = {10, 10,   10};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, false, buffer_size, max_size);
      }

      {
	/* Slices which are not in ascending order */
	int nr_slices = 2;
	hsize_t start[] = {800, 300};
	hsize_t count[] = {100, 100};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, false, buffer_size, max_size);
      }

      {
	/* Slices which overlap */
	int nr_slices = 2;
	hsize_t start[] = {500, 100};
	hsize_t count[] = {510, 100};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, false, buffer_size, max_size);
      }

      {
	/* Multiple invalid slices */
	int nr_slices = 5;
	hsize_t start[] = {0,   500, 300, 350, 9000};
	hsize_t count[] = {500, 100, 100, 100, 1500};
	pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, false, buffer_size, max_size);
      }
    }
  }

  {
    /* Read all of the elements in one big slice but with too small a buffer for even a single int */
    int nr_slices = 1;
    hsize_t start[] = {0};
    hsize_t count[] = {10000};
    pack_multiple_slices_1d(dataset_id, /* rank= */ 1, nr_slices, start, count, false, 3, 0);
  }

  H5Fclose(file_id);
  return 0;
}
