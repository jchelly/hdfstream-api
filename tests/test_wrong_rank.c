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
  (void) argv;

  /* Create a test dataset */
  char *filename = argv[1];
  hid_t file_id = create_file(filename);
  char datasetname[] = "test_dataset";
  int rank = 3;
  hsize_t dims[] = {10,10,10};
  create_dataset(file_id, datasetname, rank, dims, H5T_NATIVE_INT, fill_data);
  H5Fclose(file_id);

  /* Initialize process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(1, executable, 10, 10, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

  /* Compute minimum buffer size needed for this to work */
  size_t buffer_size_needed = sizeof(int)*dims[1]*dims[2];

  /* Try reading with various (mostly wrong) numbers of dimensions */
  hsize_t start[] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
  hsize_t count[] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
  for(int try_rank=0; try_rank<10; try_rank+=1) {

    /* Try to read the data - should only work if we used the right rank */
    size_t buffer_size = 1024;
    struct ndarray res = receive_ndarray(hs, filename, datasetname, try_rank, start, count, buffer_size);

    /* Determine whether this should have worked */
    bool succeeds = (try_rank == rank);
    assert(buffer_size > buffer_size_needed);
    if(buffer_size_needed > HDFSTREAM_MAX_BUFFER_SIZE)succeeds = false;

    /* Check the result */
    verify((res.status == 0) == succeeds);
    if(res.status == 0) {
      verify(res.rank==rank);
      int *ptr = (int *) res.data;
      for(int i=0; i<rank; i+=1)
        verify(res.shape[i] == dims[i]);
      verify(res.type[1] == 'i');
      verify(res.data_len == dims[0]*dims[1]*dims[2]*sizeof(int));
      for(int i=0; i<((int) (dims[0]*dims[1]*dims[2])); i+=1)
        verify(ptr[i] == i);
      free(res.data);
    }
  }

  hdfstream_free(hs);

  return 0;
}
