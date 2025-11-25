#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <msgpack.h>
#include <pthread.h>

#include "verify.h"
#include "hdfstream.h"
#include "create_test_file.h"
#include "receive_ndarray.h"

/*
  Test having many threads access a single dataset in a single file simultaneously
*/
const int nr_threads = 128;
const int nr_processes = 16;


static void fill_data(int rank, hsize_t *dims, void *data) {

  int *ptr = (int *) data;

  size_t nr_elements = 1;
  for(int i=0; i<rank; i+=1)
    nr_elements *= dims[i];

  for(size_t i=0; i<nr_elements; i+=1)
    ptr[i] = (int) i;
}


struct thread_data {
  int thread_nr;
  struct hdfstream *hs;
  char *filename;
  char *datasetname;
  int rank;
  hsize_t *dims;
};


/*
  Function to be executed on each thread. Reads the dataset repeatedly.
*/
static void *read_dataset(void *data) {

  struct thread_data *tdata = (struct thread_data *) data;

  unsigned int seed = tdata->thread_nr;
  const int nr_reads = 100;
  for(int i=0; i<nr_reads; i+=1) {

    /* Choose a random slice to read */
    hsize_t i1 = rand_r(&seed) % (tdata->dims[0]);
    hsize_t i2 = rand_r(&seed) % (tdata->dims[0] - i1);
    verify(i1 < tdata->dims[0]);
    verify(i1 + i2 <= tdata->dims[0]);
    fprintf(stderr, "%d, %d\n", (int) i1, (int) i2);

    /* Read the dataset slice */
    int rank = 1;
    hsize_t start[] = {i1};
    hsize_t count[] = {i2};
    size_t buffer_size = 1024;
    struct ndarray res = receive_ndarray(tdata->hs, tdata->filename, tdata->datasetname, rank, start, count, buffer_size);
    verify(res.status==0);

    /* Sanity check the result */
    verify(res.rank==1);
    verify(res.shape[0] == count[0]);
    verify(res.type[1] == 'i');
    verify(res.data_len == count[0]*sizeof(int));
    int *ptr = (int *) res.data;
    for(hsize_t j=0; j<count[0]; j+=1)
      verify((hsize_t) ptr[j] == j + start[0]);
    free(res.data);

  }

  return NULL;
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
  struct hdfstream *hs = hdfstream_new_with_executable(nr_processes, executable, 10, 10, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

  /* Read the dataset on more threads than we have reader processes */
  struct thread_data tdata[nr_threads];
  pthread_t thread[nr_threads];
  for(int i=0; i<nr_threads; i+=1) {
    tdata[i].thread_nr = i;
    tdata[i].hs = hs;
    tdata[i].filename=filename;
    tdata[i].datasetname=datasetname;
    tdata[i].rank = rank;
    tdata[i].dims = dims;
    int err = pthread_create(&thread[i], NULL, read_dataset, &tdata[i]);
    verify(err==0);
  }
  for(int i=0; i<nr_threads; i+=1) {
    void *retval;
    int err = pthread_join(thread[i], &retval);
    verify(err==0);
  }

  hdfstream_free(hs);

  return 0;
}
