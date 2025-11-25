#include <stdlib.h>
#include <stdio.h>
#include <hdf5.h>
#include <msgpack.h>

#include "file_cache.h"
#include "worker_process.h"
#include "pack_object.h"
#include "pack_dataset.h"
#include "pack_group.h"
#include "msgpack_chunkbuffer.h"
#include "commands.h"
#include "shared_memory.h"
#include "shared_data.h"

static struct file_cache *fc = NULL;
static struct shared_memory *sm = NULL;
static struct shared_reader_data *srd = NULL;

/*
  Read a dataset slice and write msgpack serialized chunks to shared memory.

  Receives input parameters via pipe to stdin.
*/
static void do_read_dataset(void) {

  int result = -1;
  char *filename = NULL;
  char *datasetname = NULL;
  hsize_t *start = NULL;
  hsize_t *count = NULL;
  int free_chunk_buffer = 0;

  /* Receive the filename */
  size_t len;
  worker_recv(sizeof(size_t), &len);
  filename = malloc(len);
  worker_recv(len, filename);

  /* Receive the dataset name */
  worker_recv(sizeof(size_t), &len);
  datasetname = malloc(len);
  worker_recv(len, datasetname);

  /* Receive the number of slices, rank, start and count parameters */
  int rank, nr_slices;
  worker_recv(sizeof(int), &rank);
  worker_recv(sizeof(int), &nr_slices);
  start = malloc(sizeof(hsize_t)*rank*nr_slices);
  count = malloc(sizeof(hsize_t)*rank*nr_slices);
  worker_recv(rank*nr_slices*sizeof(hsize_t), start);
  worker_recv(rank*nr_slices*sizeof(hsize_t), count);

  /* Receive the buffer size */
  size_t buffer_size;
  worker_recv(sizeof(size_t), &buffer_size);
  if(buffer_size > srd->buffer_size)buffer_size = srd->buffer_size;

  /* Open the file */
  int status = 0;
  struct file_cache_entry *file = file_cache_open_file(fc, filename);
  if(file) {
    status |= RESULT_FILE_OPENED;
  } else {
    worker_send(sizeof(int), &status);
    goto cleanup;
  }

  /* Open the dataset */
  hid_t dataset_id = file_cache_open_dataset(file, datasetname);
  srd->nr_file_cache_hits = fc->nr_cache_hits;
  srd->nr_file_cache_misses = fc->nr_cache_misses;
  if(dataset_id >= 0) {
    status |= RESULT_DATASET_OPENED;
  } else {
    worker_send(sizeof(int), &status);
    goto cleanup;
  }

  /* Report whether we opened the file and dataset */
  worker_send(sizeof(int), &status);

  /* Initialize output buffer */
  struct msgpack_chunkbuffer cb;
  if(msgpack_chunkbuffer_init(&cb, buffer_size, sm->data, srd) != 0)goto cleanup;
  free_chunk_buffer = 1;

  /* Initialize msgpack packer */
  struct msgpack_packer pk;
  msgpack_packer_init(&pk, &cb, msgpack_chunkbuffer_write);

  /* Pack the dataset */
  if(pack_dataset_multi_slice(dataset_id, rank, nr_slices, start, count, buffer_size, pk) == 0)
    result = 0;
  else
    result = -1;

 cleanup:

  if(filename)free(filename);
  if(datasetname)free(datasetname);
  if(start)free(start);
  if(count)free(count);
  if(free_chunk_buffer)msgpack_chunkbuffer_destroy(&cb, result);

}


/*
  Recursively read in HDF5 object structure and serialize to msgpack

  Receives object to read via pipe to stdin.
*/
static void do_read_object(void) {

  int result = -1;
  char *file_name = NULL;
  char *object_name = NULL;
  int free_chunk_buffer = 0;

  /* Receive the filename */
  size_t len;
  worker_recv(sizeof(size_t), &len);
  file_name = malloc(len);
  worker_recv(len, file_name);

  /* Receive the object name */
  worker_recv(sizeof(size_t), &len);
  object_name = malloc(len);
  worker_recv(len, object_name);

  /* Receive the maximum recursion depth */
  int max_depth;
  worker_recv(sizeof(int), &max_depth);

  /* Receive dataset size limit */
  size_t data_size_limit;
  worker_recv(sizeof(size_t), &data_size_limit);

  /* Receive the buffer size */
  size_t buffer_size;
  worker_recv(sizeof(size_t), &buffer_size);
  if(buffer_size > srd->buffer_size)buffer_size = srd->buffer_size;

  /* Try to open the file */
  int status = 0;
  struct file_cache_entry *file = file_cache_open_file(fc, file_name);
  srd->nr_file_cache_hits = fc->nr_cache_hits;
  srd->nr_file_cache_misses = fc->nr_cache_misses;
  if(file) {
    status |= RESULT_FILE_OPENED;
  } else {
    worker_send(sizeof(int), &status);
    goto cleanup;
  }

  /* Report whether we opened the file */
  worker_send(sizeof(int), &status);

  /* Initialize chunk buffer */
  struct msgpack_chunkbuffer cb;
  if(msgpack_chunkbuffer_init(&cb, buffer_size, sm->data, srd) != 0)goto cleanup;
  free_chunk_buffer = 1;

  /* Initialize msgpack packer */
  struct msgpack_packer pk;
  msgpack_packer_init(&pk, &cb, msgpack_chunkbuffer_write);

  /* Pack the object */
  if(pack_object(file->file_id, object_name, pk, max_depth, data_size_limit, buffer_size) == 0)
    result = 0;
  else
    result = 1;

 cleanup:

  if(file_name)free(file_name);
  if(object_name)free(object_name);
  if(free_chunk_buffer)msgpack_chunkbuffer_destroy(&cb, result);
  return;
}

/*
  Receive commands on stdin and write results to stdout.
*/
int main(int argc, char *argv[]) {

  if(argc != 5) {
    return 1;
  }
  int max_open_files = atoi(argv[1]);
  int max_open_datasets = atoi(argv[2]);
  int file_cache_check_interval = atoi(argv[3]);
  int file_cache_expiry_interval = atoi(argv[4]);

  if((max_open_files < 1 ) || (max_open_datasets < 1))return 1;
  if(file_cache_check_interval < 0)return 1;
  if(file_cache_expiry_interval < 0)return 1;

  /* Suppress HDF5 error output */
  H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

  worker_init();
  fc = file_cache_new(max_open_files, max_open_datasets);

  /* Receive shared memory info */
  size_t total_size;
  worker_recv(sizeof(size_t), &total_size);
  size_t len;
  worker_recv(sizeof(size_t), &len);
  char *name = malloc(len);
  worker_recv(len, name);

  /* Map the shared memory region */
  sm = shared_memory_map(name, total_size);
  assert(sm);
  free(name);

  /* Send back acknowledgement */
  int ack = 0;
  worker_send(sizeof(int), &ack);

  /* Locate shared data struct for this process */
  srd = ((struct shared_reader_data *) sm->data) + /* process_nr = */ 0;

  int done = 0;
  do {

    /* Wait to receive a command */
    int command = -1;
    int result;
    if((file_cache_check_interval > 0) && (file_cache_expiry_interval > 0)) {
      /* Cache expiry is enabled, so read with a timeout */
      result = worker_recv_with_timeout(sizeof(int), &command, file_cache_check_interval*1000);
      /* If the read timed out, check for expired cache entries */
      if(result > 0) {
        file_cache_expire_entries(fc, file_cache_expiry_interval);
        continue;
      }
    } else {
      /* Cache expiry is disabled, so block until we receive something */
      result = worker_recv(sizeof(int), &command);
    }

    /* Check for read error (e.g. broken pipe) */
    if(result < 0)exit(1);

    /* Otherwise we should have a command code */
    switch(command) {
    case COMMAND_OPEN_DATASET:
      do_read_dataset();
      break;
    case COMMAND_OPEN_OBJECT:
      do_read_object();
      break;
    case COMMAND_EXIT:
      done = 1;
      break;
    default:
      fprintf(stderr, "Unrecognised command code: %d\n", command);
      abort();
    }

  } while(done==0);

  file_cache_free(fc);
  shared_memory_unmap(sm);

  return 0;
}
