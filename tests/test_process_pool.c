#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "verify.h"
#include "process_pool.h"


struct process_pool *pool = NULL;


static void worker_process(void) {

  worker_init();

  int pid = (int) getpid();

  while(1){
    int i;
    worker_recv(sizeof(int), &i);
    fprintf(stderr, "Process %d received value %d\n", pid, i);
    worker_send(sizeof(int), &i);
  };

}


static void *thread_func(void *data) {

  int i = (intptr_t) data;

  struct worker_process *worker = process_pool_get_worker(pool);
  verify(worker);
  worker_process_send(worker, sizeof(int), &i);
  worker_process_recv(worker, sizeof(int), &i);
  process_pool_release_worker(pool, worker);

  return NULL;
}


static void manager_process(void) {

  /* Specify executable to run */
  char executable[] = "./test_process_pool";
  char *args[] = {"test_process_pool", "1", NULL};

  /* Start up the process pool */
  const int nr_processes = 4;
  pool = process_pool_new(nr_processes, executable, args, NULL, worker_default_init, NULL);
  verify(pool);

  /*
    Start up threads which will call the worker processes

    Each thread uses a process from the pool to write out its index.
    There are more threads than processes so some will need to wait.
  */
  const int nr_threads = 20;
  pthread_t thread[nr_threads];
  for(int i=0; i<nr_threads; i+=1)
    pthread_create(&thread[i], NULL, thread_func, (void *) ((intptr_t) i));
  for(int i=0; i<nr_threads; i+=1)
    pthread_join(thread[i], NULL);

  /* Free the process pool */
  process_pool_free(pool);
}


int main(int argc, char *argv[]) {

  (void) argv;

  if(argc == 1) {
    /* No arguments, so assume this is manager process */
    manager_process();
  } else {
    /* Have arguments, so assume this is a worker process */
    worker_process();
  }

  return 0;
}
