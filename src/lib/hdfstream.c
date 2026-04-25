#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <hdf5.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "process_pool.h"
#include "worker_process.h"
#include "hdfstream.h"
#include "commands.h"
#include "shared_data.h"
#include "reader_data.h"
#include "verify.h"

/* Prevent multiple threads executing hdfstream_new() simultaneously */
static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

/* Data to pass into cache_score callback */
struct callback_data {
  const char *file_name;
  const char *dataset_name;
};

/*
  Compute a score used to decide which worker process carries out a request.

  Possible results:

  0 - process does not have file or dataset cached
  1 - process has file cached
  2 - process has file and dataset cached
*/
static int cache_score(struct worker_process *worker, void *data) {

  /* Unpack input */
  struct reader_data *rd = worker->data;
  struct callback_data *cbd = (struct callback_data *) data;
  const char *file_name = cbd->file_name;
  const char *dataset_name = cbd->dataset_name;

  /* Check the cache */
  int have_file, have_dataset;
  file_cache_query(rd->file_cache, file_name, dataset_name, &have_file, &have_dataset);

  /* Compute and return score */
  int result = 0;
  if(have_file)result += 1;
  if(have_dataset)result += 1;
  return result;
}

/*
  Initialise HDF5 reader process pool
*/
struct hdfstream *hdfstream_new_with_executable(const int nr_processes, const char *executable,
                                                const int max_open_files, const int max_open_datasets,
                                                const int file_cache_check_interval,
                                                const int file_cache_expiry_interval) {

  /* Acquire the global lock. Letting multiple threads fork() at once might be
     bad and the shared memory file must be unlinked before any other thread
     tries to use it because the filename is not unique between threads. */
  pthread_mutex_lock(&global_lock);

  struct hdfstream *hs = malloc(sizeof(struct hdfstream));
  hs->max_open_files = max_open_files;
  hs->max_open_datasets = max_open_datasets;

  /* Determine executable to use */
  if(executable) {
    size_t len = strlen(executable);
    hs->executable = malloc(len+1);
    strcpy(hs->executable, executable);
  } else {
    const char *default_executable = HDFSTREAM_EXECUTABLE ;
    size_t len = strlen(default_executable);
    hs->executable = malloc(len+1);
    strcpy(hs->executable, default_executable);
  }

  /* Command line args for reader process */
  char max_files_str[100];
  sprintf(max_files_str, "%d", max_open_files);
  char max_datasets_str[100];
  sprintf(max_datasets_str, "%d", max_open_datasets);
  char file_cache_check_interval_str[100];
  sprintf(file_cache_check_interval_str, "%d", file_cache_check_interval);
  char file_cache_expiry_interval_str[100];
  sprintf(file_cache_expiry_interval_str, "%d", file_cache_expiry_interval);

  /* Start up the process pool */
  char *const args[] = {hs->executable, max_files_str, max_datasets_str,
    file_cache_check_interval_str, file_cache_expiry_interval_str, NULL};
  hs->pool = process_pool_new(nr_processes, hs->executable, args, hs, reader_init, reader_shutdown);
  if(!hs->pool) {
    free(hs->executable);
    free(hs);
    pthread_mutex_unlock(&global_lock);
    return NULL;
  }

  /* If that worked, return a new hdfstream object */
  pthread_mutex_unlock(&global_lock);
  return hs;
}

/*
  Open a set of dataset slices to read
*/
struct data_stream *hdfstream_dataset_multi_slice_open(struct hdfstream *hs, const char *file_name,
                                                       const char *dataset_name, const int nr_slices,
                                                       const int rank, const hsize_t *start,
                                                       const hsize_t *count, size_t buffer_size) {

  /*
    Do some sanity checks on parameters
  */
  if(!file_name)return NULL;
  if(!dataset_name)return NULL;
  if((rank < 0) || (rank > HDFSTREAM_MAX_DIMS))return NULL;
  if((nr_slices < 1) || (nr_slices > HDFSTREAM_MAX_SLICES))return NULL;
  if(!start)return NULL;
  if(!count)return NULL;
  if((buffer_size==0) || (buffer_size>HDFSTREAM_MAX_BUFFER_SIZE))return NULL;

  /* Assign a worker process to do the read */
  struct callback_data cbd;
  cbd.file_name = file_name;
  cbd.dataset_name = dataset_name;
  struct worker_process *worker = process_pool_get_worker_by_score(hs->pool, cache_score, &cbd);
  if(!worker)return NULL;

  /* Allocate struct to return */
  struct data_stream *stream = malloc(sizeof(struct data_stream));

  /* Locate the reader data struct for this process */
  struct reader_data *rd = worker->data;

  /* Store information needed by the stream */
  stream->worker = worker;
  stream->pool = hs->pool;
  stream->ended = 0;
  stream->next_buffer = 0;
  struct shared_reader_data *srd = ((struct shared_reader_data *) rd->sm->data);
  stream->srd = srd;
  srd->status = 0;
  stream->shared_data = rd->sm->data;

  /* Initialize semaphores */
  shared_data_init_semaphores(srd);

  /* Send read dataset command code */
  int command = COMMAND_OPEN_DATASET;
  check(worker_process_send(stream->worker, sizeof(int), &command));

  /* Send file and dataset names to worker process */
  size_t len = strlen(file_name) + 1;
  check(worker_process_send(stream->worker, sizeof(size_t), &len));
  check(worker_process_send(stream->worker, len, file_name));
  len = strlen(dataset_name) + 1;
  check(worker_process_send(stream->worker, sizeof(size_t), &len));
  check(worker_process_send(stream->worker, len, dataset_name));

  /* Send start and count arrays specifying dataset slice */
  check(worker_process_send(stream->worker, sizeof(int), &rank));
  check(worker_process_send(stream->worker, sizeof(int), &nr_slices));
  check(worker_process_send(stream->worker, nr_slices*rank*sizeof(hsize_t), start));
  check(worker_process_send(stream->worker, nr_slices*rank*sizeof(hsize_t), count));

  /* Send buffer size. This limits the size of the data blocks in the response */
  check(worker_process_send(stream->worker, sizeof(size_t), &buffer_size));

  /* Receive return code indicating if file and dataset were opened */
  int retcode;
  check(worker_process_recv(stream->worker, sizeof(int), &retcode));

  /* Update the shadow copy of this worker's cache if necessary */
  if(retcode & RESULT_FILE_OPENED) {
    struct file_cache_entry *file = file_cache_open_file(rd->file_cache, file_name);
    if(retcode & RESULT_DATASET_OPENED) {
      file_cache_open_dataset(file, dataset_name);
    }
  }

  /* If we couldn't open both the file and the dataset, there's no data stream to read */
  if(retcode != (RESULT_FILE_OPENED | RESULT_DATASET_OPENED))goto cleanup;

  /* Success! */
  return stream;

 cleanup:
  process_pool_release_worker(hs->pool, stream->worker);
  free(stream);
  return NULL;
}

/*
  Open an object to read
*/
struct data_stream *hdfstream_object_open(struct hdfstream *hs, const char *file_name,
                                          const char *object_name, const int max_depth,
                                          const size_t buffer_size, const size_t data_size_limit) {
  /*
    Do some sanity checks on parameters
  */
  if(!file_name)return NULL;
  if(!object_name)return NULL;
  if(max_depth < 0)return NULL;
  if((buffer_size==0) || (buffer_size>HDFSTREAM_MAX_BUFFER_SIZE))return NULL;

  /* Assign a worker process to do the read */
  struct callback_data cbd;
  cbd.file_name = file_name;
  cbd.dataset_name = NULL;
  struct worker_process *worker = process_pool_get_worker_by_score(hs->pool, cache_score, &cbd);
  if(!worker)return NULL;

  /* Allocate struct to return */
  struct data_stream *stream = malloc(sizeof(struct data_stream));

  /* Locate the reader data struct for this process */
  struct reader_data *rd = worker->data;

  /* Store information needed by the stream */
  stream->worker = worker;
  stream->pool = hs->pool;
  stream->ended = 0;
  stream->next_buffer = 0;
  struct shared_reader_data *srd = ((struct shared_reader_data *) rd->sm->data);
  stream->srd = srd;
  srd->status = 0;
  stream->shared_data = rd->sm->data;

  /* Initialize semaphores */
  shared_data_init_semaphores(srd);

  /* Send read object command code */
  int command = COMMAND_OPEN_OBJECT;
  check(worker_process_send(stream->worker, sizeof(int), &command));

  /* Send file and dataset names to worker process */
  size_t len = strlen(file_name) + 1;
  check(worker_process_send(stream->worker, sizeof(size_t), &len));
  check(worker_process_send(stream->worker, len, file_name));
  len = strlen(object_name) + 1;
  check(worker_process_send(stream->worker, sizeof(size_t), &len));
  check(worker_process_send(stream->worker, len, object_name));

  /* Send recursion limit */
  check(worker_process_send(stream->worker, sizeof(int), &max_depth));

  /* Send data size limit */
  check(worker_process_send(stream->worker, sizeof(size_t), &data_size_limit));

  /* Send buffer size. This limits the size of the data blocks in the response */
  check(worker_process_send(stream->worker, sizeof(size_t), &buffer_size));

  /* Receive return code indicating if file was opened */
  int retcode;
  check(worker_process_recv(stream->worker, sizeof(int), &retcode));

  /* Update the shadow copy of this worker's cache if necessary */
  if(retcode & RESULT_FILE_OPENED)
    file_cache_open_file(rd->file_cache, file_name);

  /* If we couldn't open the file, there's no data stream to read */
  if(!(retcode & RESULT_FILE_OPENED))goto cleanup;

  /* Success! */
  return stream;

 cleanup:
  process_pool_release_worker(hs->pool, stream->worker);
  free(stream);
  return NULL;
}

/*
  Read the next chunk of a data stream into the supplied buffer.
*/
size_t hdfstream_read_chunk(struct data_stream *stream, void *buffer, int *status) {

  /* Check if we already reached the end of the stream */
  if(stream->ended)return 0;

  /* Get a pointer to the next shared buffer to read */
  struct shared_reader_data *srd = stream->srd;
  struct buffer_data *shared_buffer = srd->buffer + stream->next_buffer;

  /* Wait until the shared buffer is ready to read */
  if(worker_process_wait_for_semaphore(stream->worker, &(shared_buffer->ready_for_read), /* delay = */ 1) < 0) {
    /* Reader process failed somehow */
    *status = BUFFER_STATUS_ERROR;
    stream->ended = 1;
    return 0;
  }

  /* Read the length of the block */
  size_t block_len = shared_buffer->used;

  if(block_len == 0) {
    /* Zero size block indicates end of stream */
    *status = shared_buffer->status;
    stream->ended = 1;
    return 0;
  } else {
    /* Non-zero means we successfully read some data */
    *status = 0;
  }

  /* Copy the data to the output buffer */
  if(buffer)memcpy(buffer, stream->shared_data+shared_buffer->offset, block_len);

  /* Reset this buffer to empty */
  shared_buffer->used = 0;

  /* Flag this buffer as available to over write */
  sem_post(&(shared_buffer->ready_for_write));

  /* Advance to the buffer to read on the next call */
  stream->next_buffer = (stream->next_buffer + 1) % NR_BUFFERS;

  return block_len;
}

/*
  Deallocate a data stream and return the worker process to the pool

  This can be called without reading all of the data if we want to
  end the stream early.
*/
void hdfstream_close_stream(struct data_stream *stream) {

  if(!stream->ended) {
    /* If we didn't read all of the data, signal reader to stop and wait until it does */
    struct shared_reader_data *srd = stream->srd;
    srd->status = BUFFER_STATUS_CANCEL;
    int status;
    while(hdfstream_read_chunk(stream, NULL, &status) > 0) {};
  }

  process_pool_release_worker(stream->pool, stream->worker);
  free(stream);
}

/*
  Report cache stats.

  Use stats collected on worker process because these include effects of
  cache expiry.
*/
struct cache_info hdfstream_cache_info(struct hdfstream *hs, const int worker_nr) {

  struct cache_info ci;
  if((worker_nr >= 0) && (worker_nr < hs->pool->max_nr_processes)) {
    ci.process_state = hs->pool->worker_state[worker_nr];
    if((ci.process_state == IDLE) || (ci.process_state == BUSY)) {
      assert(hs->pool->worker[worker_nr]);
      struct reader_data *rd = hs->pool->worker[worker_nr]->data;
      struct shared_reader_data *srd = ((struct shared_reader_data *) rd->sm->data);
      ci.nr_file_cache_hits = srd->nr_file_cache_hits;
      ci.nr_file_cache_misses = srd->nr_file_cache_misses;
    } else {
      /* Dead process, so no cache info */
      ci.nr_file_cache_hits = 0;
      ci.nr_file_cache_misses = 0;
    }
  } else {
    ci.process_state = -1;
    ci.nr_file_cache_hits = -1;
    ci.nr_file_cache_misses = -1;
  }
  return ci;
}

/*
  Shut down and terminate all reader processes
*/
void hdfstream_free(struct hdfstream *hs) {

  process_pool_free(hs->pool);
  if(hs->executable)free(hs->executable);
  free(hs);
}
