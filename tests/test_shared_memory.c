#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "verify.h"
#include "worker_process.h"
#include "shared_memory.h"

/* Shared memory region name and size in bytes */
static const char name[] = "/hdfstream_sm_test";
static const size_t size = 1024*1024;


static void manager_process(void) {

  /* Specify executable to run */
  const char *executable = "./test_shared_memory";
  char *const args[] = {"test_shared_memory", "1", NULL};

  /* Create a shared memory region */
  struct shared_memory *sm = shared_memory_new(name, size);
  verify(sm);

  /* Populate shared memory region */
  int n = size / sizeof(int);
  for(int i=0; i<n; i+=1) {
    int *ptr = (int *) sm->data;
    ptr[i] = i*100;
  }

  /* Start the worker process */
  struct worker_process *worker = worker_process_new(executable, args, NULL, NULL, NULL);
  verify(worker);

  /* Wait for worker to map shared memory region */
  int i = 0;
  worker_process_recv(worker, sizeof(int), &i);
  verify(i==1);

  /* Unlink the shared memory file */
  verify(shared_memory_unlink(sm)==0);

  /* Send shutdown signal to the worker process */
  i = 2;
  worker_process_send(worker, sizeof(int), &i);

  /* Tidy up */
  worker_process_free(worker);
  shared_memory_unmap(sm);
}


static void worker_process(void) {

  worker_init();

  /* Map the shared memory region */
  struct shared_memory *sm = shared_memory_map(name, size);
  verify(sm);

  /* Check that shared region has the expected contents */
  int *ptr = (int *) sm->data;
  int n = size / sizeof(int);
  for(int i=0; i<n; i+=1) {
    verify(ptr[i] == i*100);
  }

  /* Signal that we mapped and verified the region */
  int i = 1;
  worker_send(sizeof(int), &i);

  /* Receive shutdown signal */
  worker_recv(sizeof(int), &i);
  verify(i==2);

  /* Tidy up */
  shared_memory_unmap(sm);
}


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  if(argc == 1) {
    /* No arguments, so assume this is manager process */
    manager_process();
  } else {
    /* Have arguments, so assume this is worker process */
    worker_process();
  }
}
