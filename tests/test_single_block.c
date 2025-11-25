#include <stdlib.h>
#include <stdio.h>
#include <msgpack.h>

#include "verify.h"
#include "hdfstream.h"
#include "create_test_file.h"
#include "receive_ndarray.h"

/*
  Test the case where we read exactly the right number of data chunks,
  so that we don't explicitly receive the terminating zero size block.
*/

static void fill_data(int rank, hsize_t *dims, void *data) {

  int *ptr = (int *) data;

  size_t nr_elements = 1;
  for(int i=0; i<rank; i+=1)
    nr_elements *= dims[i];

  for(size_t i=0; i<nr_elements; i+=1)
    ptr[i] = (int) i;
}


int main(int argc, char *argv[]) {

  (void) argc;

  /* Create a test dataset */
  char *filename = argv[1];
  hid_t file_id = create_file(filename);
  char datasetname[] = "test_dataset";
  int rank = 1;
  hsize_t dims[] = {10000};
  create_dataset(file_id, datasetname, rank, dims, H5T_NATIVE_INT, fill_data);
  H5Fclose(file_id);

  /* Initialize process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(1, executable, 10, 10, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

  size_t buffer_size;
  hsize_t start[] = {0};
  hsize_t count[] = {dims[0]};
  struct data_stream *stream;

  /* Here the full response fits in one block */
  buffer_size = dims[0]*sizeof(int)*2;
  stream = hdfstream_dataset_slice_open(hs, filename, datasetname,
					rank, start, count, buffer_size);
  int status;
  char *buffer = malloc(buffer_size);
  hdfstream_read_chunk(stream, buffer, &status);
  verify(status==0);
  free(buffer);

  hdfstream_close_stream(stream);

  /* Now check that we can still read a dataset successfully */
  struct ndarray res = receive_ndarray(hs, filename, datasetname, rank, start, count, buffer_size);
  verify(res.status==0);

  /* Sanity check the result */
  verify(res.rank==1);
  verify(res.shape[0] == dims[0]);
  verify(res.type[1] == 'i');
  verify(res.data_len == dims[0]*sizeof(int));
  int *ptr = (int *) res.data;
  for(int i=0; i<(int) dims[0]; i+=1)
    verify(ptr[i] == i);
  free(res.data);

  hdfstream_free(hs);

  return 0;
}
