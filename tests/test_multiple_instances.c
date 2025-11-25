#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <msgpack.h>
#include <pthread.h>

#include "verify.h"
#include "hdfstream.h"
#include "create_test_file.h"
#include "receive_ndarray.h"

#define MAX_FILENAME_LEN 1024

/*
  Test having many threads access multiple datasets in multiple files simultaneously.
  In this case every thread starts its own hdfstream instance.
*/
const int nr_files = 8;
const int nr_datasets = 12;
const int nr_threads = 4;
const int nr_processes = 2;

const int max_open_files = 4;
const int max_open_datasets = 8;

/* Will add a different offset to elements in each file to detect if we read the wrong file */
static int offset = 0;
const int dataset_offset_factor = 10000;
const int file_offset_factor = 1000000;

static void fill_data(int rank, hsize_t *dims, void *data) {

  int *ptr = (int *) data;

  size_t nr_elements = 1;
  for(int i=0; i<rank; i+=1)
    nr_elements *= dims[i];

  for(size_t i=0; i<nr_elements; i+=1)
    ptr[i] = offset + (int) i;
}


struct thread_data {
  int thread_nr;
  char *filename;
  int rank;
  hsize_t *dims;
};


/*
  Function to be executed on each thread. Reads the dataset repeatedly.
*/
static void *read_dataset(void *data) {

  struct thread_data *tdata = (struct thread_data *) data;

  /* Initialize process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(nr_processes, executable, max_open_files, max_open_datasets, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

  unsigned int seed = tdata->thread_nr;
  const int nr_reads = 5000;
  for(int i=0; i<nr_reads; i+=1) {

    /* Pick a file to read */
    char filename[MAX_FILENAME_LEN];
    int file_nr = rand_r(&seed) % nr_files;
    sprintf(filename, "%s.%d", tdata->filename, file_nr);

    /* Pick a dataset to read */
    char datasetname[MAX_FILENAME_LEN];
    int dataset_nr = rand_r(&seed) % nr_datasets;
    sprintf(datasetname, "test_dataset_%d", dataset_nr);

    printf("Thread %d reading file %d dataset %d\n", tdata->thread_nr, file_nr, dataset_nr);

    /* Choose a random slice to read */
    hsize_t i1 = rand_r(&seed) % (tdata->dims[0]);
    hsize_t i2 = rand_r(&seed) % (tdata->dims[0] - i1);
    verify(i1 < tdata->dims[0]);
    verify(i1 + i2 <= tdata->dims[0]);

    /* Read the dataset slice */
    int rank = 1;
    hsize_t start[] = {i1};
    hsize_t count[] = {i2};
    size_t buffer_size = 1024;
    struct ndarray res = receive_ndarray(hs, filename, datasetname, rank, start, count, buffer_size);
    verify(res.status==0);

    /* Sanity check the result */
    verify(res.rank==1);
    verify(res.shape[0] == count[0]);
    verify(res.type[1] == 'i');
    verify(res.data_len == count[0]*sizeof(int));
    int *ptr = (int *) res.data;
    for(hsize_t j=0; j<count[0]; j+=1)
      verify((hsize_t) ptr[j] == j + start[0] + file_offset_factor*file_nr + dataset_offset_factor*dataset_nr);
    free(res.data);
  }

  hdfstream_free(hs);

  return NULL;
}


int main(int argc, char *argv[]) {

  (void) argc;

  /* Create test datasets over multiple files */
  int rank = 1;
  hsize_t dims[] = {10000};
  for(int i=0; i<nr_files; i+=1) {

    /* Generate filename and create file */
    char filename[MAX_FILENAME_LEN];
    sprintf(filename, "%s.%d", argv[1], i);
    hid_t file_id = create_file(filename);

    /* Create test datasets */
    for(int j=0; j<nr_datasets; j+=1) {
      char datasetname[MAX_FILENAME_LEN];
      sprintf(datasetname, "test_dataset_%d", j);
      offset = file_offset_factor*i + dataset_offset_factor*j;
      create_dataset(file_id, datasetname, rank, dims, H5T_NATIVE_INT, fill_data);
    }
    H5Fclose(file_id);
  }

  /* Read the datasets on more threads than we have reader processes */
  struct thread_data tdata[nr_threads];
  pthread_t thread[nr_threads];
  for(int i=0; i<nr_threads; i+=1) {
    tdata[i].thread_nr = i;
    tdata[i].filename=argv[1];
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

  return 0;
}
