#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "verify.h"
#include "process_pool.h"

/*
  This checks that process_pool_get_worker_by_score() correctly selects the
  worker process with the highest score.
*/

struct process_pool *pool = NULL;


static void worker_process(void) {
  /*
    This is the code that will be executed by the workers.
  */
  worker_init();
  while(1){
    int i;
    worker_recv(sizeof(int), &i);
    worker_send(sizeof(int), &i);
  };

}

static int counter = 0;
static int worker_init_callback(struct worker_process *worker) {

  if(worker_default_init(worker) != 0)return -1;

  /* Allocate space for an int in the user data pointer and set a value */
  worker->data = malloc(sizeof(int));
  *((int *) worker->data) = counter;
  counter += 1;

  return 0;
}

static int worker_shutdown_callback(struct worker_process *worker) {

  free(worker->data);
  return 0;
}

static int score_func_forward(struct worker_process *worker, void *data) {
  /* Score is just the int pointed at by the user data pointer */
  (void) data;
  return *((int *) worker->data);
}

static int score_func_reverse(struct worker_process *worker, void *data) {
  /* Reverse the score ordering */
  int nr_processes = *((int *) data);
  return nr_processes - *((int *) worker->data);
}


static void manager_process(void) {

  /* Specify executable to run */
  char executable[] = "./test_process_pool_score";
  char *args[] = {"test_process_pool_score", "1", NULL};

  /* Start up the process pool */
  const int nr_processes = 4;
  pool = process_pool_new(nr_processes, executable, args, NULL, worker_init_callback, worker_shutdown_callback);
  verify(pool);

  /* Start up all processes */
  for(int i=0; i<nr_processes; i+=1)
    process_pool_start_worker(pool);

  /* Request processes: this should return them in descending score order */
  struct worker_process **worker = malloc(sizeof(struct worker_process)*nr_processes);
  for(int i=0; i<nr_processes; i+=1) {
    worker[i] = process_pool_get_worker_by_score(pool, score_func_forward, NULL);
    verify(worker[i]); // should not return null
  }

  /* Check process ordering */
  for(int i=0; i<nr_processes; i+=1) {
    int score = *((int *) worker[i]->data);
    verify(score==nr_processes-i-1);
  }

  /* Return all processes to the pool */
  for(int i=0; i<nr_processes; i+=1)
    process_pool_release_worker(pool, worker[i]);

  /* Try again, reversing the score function */
  for(int i=0; i<nr_processes; i+=1) {
    worker[i] = process_pool_get_worker_by_score(pool, score_func_reverse, (void *) &nr_processes);
    verify(worker[i]); // should not return null
  }

  /* Check process ordering */
  for(int i=0; i<nr_processes; i+=1) {
    int score = *((int *) worker[i]->data);
    verify(score==i);
  }

  /* Return all processes to the pool */
  for(int i=0; i<nr_processes; i+=1)
    process_pool_release_worker(pool, worker[i]);

  /* Tidy up */
  free(worker);
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
