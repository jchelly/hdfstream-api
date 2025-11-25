#include <stdlib.h>
#include <stdio.h>
#include <msgpack.h>

#include "verify.h"
#include "hdfstream.h"
#include "create_test_file.h"
#include "receive_ndarray.h"


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
  int rank = 0;
  hsize_t dims[] = {0};
  create_dataset(file_id, datasetname, rank, dims, H5T_NATIVE_INT, fill_data);
  H5Fclose(file_id);

  /* Initialize process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(1, executable, 10, 10, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

  /* Read the dataset*/
  hsize_t start[] = {0};
  hsize_t count[] = {0};
  size_t buffer_size = 1024;
  struct ndarray res = receive_ndarray(hs, filename, datasetname, rank, start, count, buffer_size);
  verify(res.status==0);

  /* Sanity check the result */
  verify(res.rank==0);
  verify(res.type[1] == 'i');
  verify(res.data_len == sizeof(int));
  int *ptr = (int *) res.data;
  verify(ptr[0] == 0);
  free(res.data);

  hdfstream_free(hs);

  return 0;
}
