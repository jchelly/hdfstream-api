#ifndef PROCESS_POOL_H
#define PROCESS_POOL_H

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include "worker_process.h"

typedef enum {
  IDLE,
  BUSY,
  STOPPED
} worker_state_t;

struct process_pool {
  int max_nr_processes;
  int nr_processes_running;
  char *executable;
  int nargs;
  char **args;
  struct worker_process **worker;
  worker_state_t *worker_state;
  sem_t sem;
  pthread_mutex_t mutex;
  worker_callback init;
  worker_callback shutdown;
  void *data;
  int next;
};


struct process_pool *process_pool_new(const int max_nr_processes, const char *executable, char *const args[],
                                      void *data, worker_callback init_callback,
                                      worker_callback shutdown_callback);

/* Select available worker with the highest score computed using the supplied callback */
typedef int (*score_callback)(struct worker_process *worker, void *data);
struct worker_process *process_pool_get_worker_by_score(struct process_pool *pool, score_callback cb, void *data);

/* Macro to just get the next available worker process */
#define process_pool_get_worker(pool) process_pool_get_worker_by_score(pool, NULL, NULL)

void process_pool_start_worker(struct process_pool *pool);
void process_pool_release_worker(struct process_pool *pool, struct worker_process *worker);
void process_pool_free(struct process_pool *pool);
void process_pool_wait_and_lock(struct process_pool *pool);
void process_pool_unlock(struct process_pool *pool);

#endif
