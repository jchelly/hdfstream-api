#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "hdfstream.h"
#include "worker_process.h"
#include "reader_data.h"
#include "shared_data.h"
#include "verify.h"

#define MAX_SM_NAME_LENGTH 1024

/*
  Callback called on the parent process when a new reader child process is
  created.

  - allocates a shared memory segment for communication with the child
  - allocates a copy of the child process file cache
*/
int reader_init(struct worker_process *worker) {

  /* Get pointer to the hdfstream struct */
  struct hdfstream *hs = worker->data;
  worker->data = NULL;

  /* Check for successful process startup */
  if(worker_default_init(worker) != 0)return -1;

  /* Allocate reader data for this process */
  worker->data = malloc(sizeof(struct reader_data));
  if(!worker->data)return -1;
  struct reader_data *rd = worker->data;

  /* Initialize struct components to make cleanup on failure easier */
  rd->sm = NULL;
  rd->file_cache = NULL;

  /* Generate a unique name for the shared memory region for this process */
  char name[MAX_SM_NAME_LENGTH];
  int parent_pid = (int) getpid();
  int child_pid = worker->pid;
  snprintf(name, MAX_SM_NAME_LENGTH, "/hdfstream_%d_%d", parent_pid, child_pid);

  /* Set up the shared memory region */
  size_t total_size = shared_data_size(HDFSTREAM_MAX_BUFFER_SIZE);
  rd->sm = shared_memory_new(name, total_size);
  if(!rd->sm)return -1;

  /* Populate the shared memory region */
  shared_data_populate(rd->sm->data, HDFSTREAM_MAX_BUFFER_SIZE);

  /* Send shared memory name and size to the child process and wait for acknowledgement */
  size_t len = strlen(name) + 1;
  check(worker_process_send(worker, sizeof(size_t), &total_size));
  check(worker_process_send(worker, sizeof(size_t), &len));
  check(worker_process_send(worker, len, name));
  int ack = 1;
  check(worker_process_recv(worker, sizeof(int), &ack)); /* Indicates subprocess has mapped shared memory */
  if(ack != 0)goto cleanup;

  /* Allocate "shadow" copy of file cache */
  rd->file_cache = file_cache_new(hs->max_open_files, hs->max_open_datasets);
  if(!rd->file_cache)goto cleanup;

  /* We can unlink the shared file now that the process has mapped it */
  shared_memory_unlink(rd->sm);

  return 0;

 cleanup:
  /* Ensure we unlink the shared memory on failure. Other cleanup is done by the shutdown function */
  if(rd->sm)shared_memory_unlink(rd->sm);
  return -1;
}

/*
  Callback called on the parent process when a reader child process is shut
  down. Also called if the startup function fails.
*/
int reader_shutdown(struct worker_process *worker) {

  struct reader_data *rd = worker->data;
  if(rd) {
    if(rd->sm) {
      shared_data_free(rd->sm->data);
      shared_memory_unmap(rd->sm);
    }
    if(rd->file_cache)file_cache_free(rd->file_cache);
    if(rd)free(rd);
  }
  return 0;
}
