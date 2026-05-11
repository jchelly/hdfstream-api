#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "verify.h"
#include "process_pool.h"


struct process_pool *pool = NULL;


static void worker_process(void) {

  worker_init();

  while(1){
    int i;
    worker_recv(sizeof(int), &i);
    worker_send(sizeof(int), &i);
  };

}


static void *thread_func(void *data) {

  int i = (intptr_t) data;

  while(1) {
    struct worker_process *worker = process_pool_get_worker(pool);
    if(!worker){
      break;
    }
    /* Thread may or may not communicate with the process */
    if(i % 3 > 0) {
      worker_process_send(worker, sizeof(int), &i);
      worker_process_recv(worker, sizeof(int), &i);
    }
    process_pool_release_worker(pool, worker);
  };
  return NULL;
}


static void manager_process(const int nr_processes, const int nr_threads) {

  /* Specify executable to run */
  char executable[] = "./test_process_pool_shutdown";
  char *args[] = {"test_process_pool_shutdown", "1", NULL};

  /* Start up the process pool */
  pool = process_pool_new(nr_processes, executable, args, NULL, worker_default_init, NULL);
  verify(pool);

  /*
    Start up threads which will call the worker processes

    Each thread uses a process from the pool to write out its index.
    There are more threads than processes so some will need to wait.
  */
  pthread_t thread[nr_threads];
  for(int i=0; i<nr_threads; i+=1)
    pthread_create(&thread[i], NULL, thread_func, (void *) ((intptr_t) i));

  /* Let the threads run for a bit */
  struct timespec ts = {0, 100000000}; // 100 ms
  nanosleep(&ts, NULL);
  /* sleep(1); */

  /* Freeing the pool should cause the threads to exit */
  process_pool_free(pool);

  /* Should now be able to join all threads */
  for(int i=0; i<nr_threads; i+=1)
    pthread_join(thread[i], NULL);
}


int main(int argc, char *argv[]) {

  (void) argv;

  if(argc == 1) {
    /* No arguments, so assume this is manager process */
    for(int i=0; i<50; i+=1)
      manager_process(4, 20); /* More threads than processes */
    for(int i=0; i<50; i+=1)
      manager_process(8, 4); /* More processes than threads */
    for(int i=0; i<50; i+=1)
      manager_process(5, 5); /* Equal numbers */
  } else {
    /* Have arguments, so assume this is a worker process */
    worker_process();
  }

  return 0;
}
