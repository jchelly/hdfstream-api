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
    if(i == 1)exit(0); /* Terminate if sent a 1 */
    worker_send(sizeof(int), &i);
  };

}


static void manager_process(void) {

  /* Specify executable to run */
  char executable[] = "./test_process_pool_dead_process";
  char *args[] = {"test_process_pool_dead_process", "1", NULL};

  /* Start up the process pool */
  const int nr_processes = 1;
  pool = process_pool_new(nr_processes, executable, args, NULL, worker_default_init, NULL);
  verify(pool);

  /* Request a process and send a value that will cause it to exit */
  struct worker_process *worker = process_pool_get_worker(pool);
  int i = 1;
  worker_process_send(worker, sizeof(int), &i);
  int j;
  verify(worker_process_recv(worker, sizeof(int), &j) != 0); /* Recv should fail */

  /* Return the (dead) process to the pool */
  process_pool_release_worker(pool, worker);

  /*
    Try again with a value which will not terminate the process.
    The pool should start up a replacement automatically.
  */
  worker = process_pool_get_worker(pool);
  verify(worker);
  verify(worker->is_dead==0);
  i = 2;
  verify(worker_process_send(worker, sizeof(int), &i) == 0);
  verify(worker_process_recv(worker, sizeof(int), &j) == 0);

  /* Return the process to the pool */
  process_pool_release_worker(pool, worker);

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
