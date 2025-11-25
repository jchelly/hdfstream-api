#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "verify.h"
#include "worker_process.h"

static int init_callback(struct worker_process *worker) {

  if(worker_default_init(worker) != 0)return -1;

  /* Store an int in the worker's data pointer */
  worker->data = malloc(sizeof(int));
  int *i = (int *) worker->data;
  *i = 42;
  return 0;
}

static int shutdown_callback(struct worker_process *worker) {

  /* Check int data has been preserved */
  int *i = (int *) worker->data;
  verify(*i == 42);
  /* Tidy up */
  free(worker->data);
  return 0;
}

static void manager_process(void) {

  /* Specify executable to run */
  const char *executable = "./test_worker_callback";
  char *const args[] = {"test_worker_callback", "1", NULL};

  /* Start the process */
  struct worker_process *worker = worker_process_new(executable, args, NULL, init_callback, shutdown_callback);
  verify(worker);

  for(int i=0; i<10; i+=1) {

    /* Send an integer to the process */
    int err;
    err = worker_process_send(worker, sizeof(int), &i);
    verify(err==0);

    /* Receive an integer from the process */
    int j = -1;
    err = worker_process_recv(worker, sizeof(int), &j);
    /* fprintf(stderr, "Manager: received from worker %d\n", j); */
    verify(err==0);
    verify(j==2*i);

  }

  /* Send shutdown signal */
  int i = -1;
  worker_process_send(worker, sizeof(int), &i);

  /* Shut down */
  worker_process_free(worker);
}


static void worker_process(void) {

  worker_init();

  while(1) {

    /* Receive int on stdin */
    int j;
    worker_recv(sizeof(int), &j);
    /* fprintf(stderr, "Worker: received from manager %d\n", j); */

    if(j==-1)exit(0);

    /* Otherwise double the value and return it */
    j *= 2;
    worker_send(sizeof(int), &j);
  }

}


int main(int argc, char *argv[]) {

  (void) argv;

  if(argc == 1) {
    /* No arguments, so assume this is manager process */
    manager_process();
  } else {
    /* Have arguments, so assume this is worker process */
    worker_process();
  }

}
