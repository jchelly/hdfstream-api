#include <stdlib.h>
#include <stdio.h>
#include <msgpack.h>

#include "verify.h"
#include "hdfstream.h"
#include "create_test_file.h"
#include "receive_ndarray.h"

const int str_len = 20;

static void fill_data(int rank, hsize_t *dims, void *data) {

  char *ptr = (char *) data;

  size_t nr_elements = 1;
  for(int i=0; i<rank; i+=1)
    nr_elements *= dims[i];

  for(size_t i=0; i<nr_elements; i+=1) {
    char *element = ptr+i*str_len;
    sprintf(element, "%d", (int) i);
  }
}


int main(int argc, char *argv[]) {

  (void) argc;

  /* Make a fixed length string type */
  hid_t str_type_id = H5Tcreate(H5T_STRING, str_len);

  /* Create a test dataset */
  char *filename = argv[1];
  hid_t file_id = create_file(filename);
  char datasetname[] = "test_dataset";
  int rank = 1;
  hsize_t dims[] = {10000};
  create_dataset(file_id, datasetname, rank, dims, str_type_id, fill_data);
  H5Fclose(file_id);
  H5Tclose(str_type_id);

  /* Initialize process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(1, executable, 10, 10, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

  /* Read the dataset*/
  hsize_t start[] = {0};
  hsize_t count[] = {dims[0]};
  size_t buffer_size = 1024;
  struct ndarray res = receive_ndarray(hs, filename, datasetname, rank, start, count, buffer_size);
  verify(res.status==0);

  /* Sanity check the result */
  verify(res.rank==1);
  verify(res.shape[0] == dims[0]);
  verify(res.type[0] == 'S');
  verify(res.data_len == dims[0]*str_len);
  char *ptr = (char *) res.data;
  for(int i=0; i<((int) dims[0]); i+=1) {
    char expected[str_len];
    sprintf(expected, "%d", i);
    verify(strcmp(expected, ptr+i*str_len)==0);
  }
  free(res.data);

  hdfstream_free(hs);

  return 0;
}
