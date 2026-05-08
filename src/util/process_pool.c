#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>

#include "process_pool.h"

/* Linked list of process pools */
static pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
struct process_pool *first_pool = NULL;

/*
  Create a new process pool
*/
struct process_pool *process_pool_new(const int max_nr_processes,
                                      const char *executable, char *const args[], void *data,
                                      worker_callback init_callback, worker_callback shutdown_callback) {

  /* Allocate the pool */
  struct process_pool *pool = malloc(sizeof(struct process_pool));
  pool->next = 0;

  /* Store worker process parameters */
  pool->data = data;
  pool->init = init_callback;
  pool->shutdown = shutdown_callback;
  pool->stop = 0;

  /* Initialize semphore to control access */
  if(sem_init(&(pool->sem), 0, (unsigned int) 0) != 0) {
    free(pool);
    return NULL;
  }

  /* Initialize mutex to protect worker_state flags */
  if(pthread_mutex_init(&pool->mutex, NULL) != 0) {
    sem_destroy(&(pool->sem));
    free(pool);
    return NULL;
  }

  /* Allocate array of workers */
  pool->max_nr_processes = max_nr_processes;
  pool->worker = malloc(max_nr_processes*sizeof(struct worker_process *));
  pool->worker_state = malloc(max_nr_processes*sizeof(worker_state_t));

  /* Initially, all processes are stopped */
  for(int i=0; i<max_nr_processes; i+=1) {
    pool->worker[i] = NULL;
    pool->worker_state[i] = STOPPED;
  }
  pool->nr_processes_running = 0;

  /* Store executable name */
  pool->executable = malloc(sizeof(char)*(strlen(executable)+1));
  strcpy(pool->executable, executable);

  /* Store args, assumed to be NULL terminated array of char **/
  pool->nargs = 0;
  while(args[pool->nargs] != NULL) {
    pool->nargs += 1;
  }
  pool->args = malloc(sizeof(char *)*(pool->nargs+1));
  pool->args[pool->nargs] = NULL;
  for(int i=0; i<pool->nargs; i+=1) {
    pool->args[i] = malloc(sizeof(char)*(strlen(args[i])+1));
    strcpy(pool->args[i], args[i]);
  }

  /* Prepend the new pool to the linked list */
  pthread_mutex_lock(&pool_mutex);
  if(!first_pool)atexit(process_pool_cleanup);
  pool->next_pool = first_pool;
  first_pool = pool;
  pthread_mutex_unlock(&pool_mutex);

  return pool;
}

/*
  Try to start up a new process while not exceeding the maximum
*/
int process_pool_start_worker(struct process_pool *pool) {

  int nr_running = 0;
  pthread_mutex_lock(&pool->mutex);
  /* Check we didn't reach the max nr processes before we acquired the lock */
  if((pool->nr_processes_running < pool->max_nr_processes) && (!pool->stop)) {
    /* Find the first stopped process index */
    int i = 0;
    while(i < pool->max_nr_processes) {
      if(pool->worker_state[i]==STOPPED)break;
      i += 1;
    }
    if(i < pool->max_nr_processes) {
      /* Start a new process */
      pool->worker[i] = worker_process_new(pool->executable, pool->args, pool->data, pool->init, pool->shutdown);
      if(pool->worker[i]) {
        /* New process is running */
        pool->worker_state[i] = IDLE;
        pool->nr_processes_running += 1;
        assert(pool->nr_processes_running <= pool->max_nr_processes);
        sem_post(&(pool->sem));
      } else {
        /* Failed to start */
        pool->worker_state[i] = STOPPED;
      }
    }
  }
  nr_running = pool->nr_processes_running;
  pthread_mutex_unlock(&pool->mutex);
  return nr_running;
}

/*
  Get a process from the pool

  If the callback cb is set then we call it to compute a score for each free
  process and pick the one with the highest score. The data pointer can be
  used to pass parameters to the callback.
*/
static struct worker_process *try_process_pool_get_worker_by_score(struct process_pool *pool, score_callback cb, void *data) {

  /*
    Start a new process if necessary

    We start a process if we don't have the maximum number already running
    and there are none currently free.
  */
  int nr_running;
  int nr_available = 0;
  sem_getvalue(&pool->sem, &nr_available);
  if(nr_available <= 0) {
    nr_running = process_pool_start_worker(pool);
    /* If there are still no processes, we can't do anything */
    if(nr_running == 0)return NULL;
  }

  /* Wait until a process is available */
  process_pool_wait_and_lock(pool);

  /* Fail if we've shut down */
  if(pool->stop) {
    sem_post(&(pool->sem)); /* We didn't assign a process */
    process_pool_unlock(pool);
    return NULL;
  }

  /* Find a free process to use */
  struct worker_process *worker = NULL;
  int high_score_index = -1;
  int high_score_value = -1;
  for(int i=0; i<pool->max_nr_processes; i+=1) {
    int j = (i + pool->next) % pool->max_nr_processes;
    if(pool->worker_state[j] == IDLE) {
      assert(pool->worker[j]);
      if(!cb) {
        /* Not computing scores, so use the first free process */
        high_score_index = j;
        break;
      } else {
        /* Compute the score for this process */
        int score = cb(pool->worker[j], data);
        assert(score >= 0);
        /* Compare to best so far */
        if(score > high_score_value) {
          high_score_value = score;
          high_score_index = j;
        }
      }
    }
  }
  if(high_score_index < 0) {
    /* Should not happen: we reserved a process with sem_wait */
    assert(high_score_index >= 0);
    worker = NULL;
  } else {

    /* Assign the worker process */
    worker = pool->worker[high_score_index];
    pool->worker_state[high_score_index] = BUSY;

    /* Advance starting process to spread the load evenly */
    pool->next = (pool->next + 1) % pool->max_nr_processes;
  }

  /* Free the lock */
  process_pool_unlock(pool);

  if(worker==NULL)sem_post(&(pool->sem)); /* We didn't assign a process */

  /* Return a pointer to the worker process */
  return worker;
}

/*
  Get a process from the pool, trying again if we get a dead process.

  If a process died while idle then try_process_pool_get_worker_by_score() can
  return a dead process. If so we mark it as stopped and try again.
*/
struct worker_process *process_pool_get_worker_by_score(struct process_pool *pool, score_callback cb, void *data) {

  /* Limit number of tries to avoid infinite loop if starting always fails */
  for(int i=0; i<pool->max_nr_processes+1; i+=1) {
    struct worker_process *worker = try_process_pool_get_worker_by_score(pool, cb, data);
    if(!worker)return NULL;
    if(worker_process_is_alive(worker)) {
      /* Success */
      return worker;
    } else {
      /*
        The process we picked is not running.

        Mark it as dead, free resources, and try again. The semaphore has
        already been decremented so we don't need to update it here.
      */
      pthread_mutex_lock(&pool->mutex);
      for(int j=0; j<pool->max_nr_processes;j+=1) {
        if(worker==pool->worker[j]) {
          pool->worker_state[j] = STOPPED;
          worker_process_free(worker);
          pool->worker[j] = NULL;
          pool->nr_processes_running -= 1;
        }
      }
      pthread_mutex_unlock(&pool->mutex);
    }
  }
  return NULL;
}

/*
  Return a process to the pool
*/
void process_pool_release_worker(struct process_pool *pool, struct worker_process *worker) {

  /* Acquire the lock */
  pthread_mutex_lock(&pool->mutex);

  /* Find the process to release */
  int running = 0;
  for(int i=0; i<pool->max_nr_processes; i+=1) {
    if(worker==pool->worker[i]) {
      /* Check if it died! */
      if(worker_process_is_alive(worker)) {
        /* Still alive, so it goes back in the pool */
        pool->worker_state[i] = IDLE;
        running = 1;
      } else {
        /* It died, so free resources and set its pointer to NULL */
        pool->worker_state[i] = STOPPED;
        worker_process_free(worker);
        pool->worker[i] = NULL;
        pool->nr_processes_running -= 1;
      }
      break;
    }
  }

  /* Free the lock */
  pthread_mutex_unlock(&pool->mutex);

  /* Signal that another process is now available, if it didn't die */
  if(running)sem_post(&(pool->sem));
}

/*
  "Free" a process pool. We really just block it from starting new
  processes, wait for the current processes to exit, and free any
  resources associated with them. The pool itself cannot be freed
  because threads might still have a pointer to it.
*/
void process_pool_free(struct process_pool *pool) {

  pthread_mutex_lock(&pool->mutex);
  /* Block starting of new processes */
  pool->stop = 1;
  /* Signal all processes to stop */
  for(int i=0; i<pool->max_nr_processes; i+=1) {
    if(pool->worker[i])worker_process_kill(pool->worker[i]);
  }
  /* Make sure threads don't block at the semaphore:
     we might have shut down at a moment when all processes
     were allocated. */
  sem_post(&(pool->sem));
  pthread_mutex_unlock(&pool->mutex);

  /* Wait until all worker processes are released */
  while(1) {
    pthread_mutex_lock(&pool->mutex);
    int nr_busy = 0;
    for(int i=0; i<pool->max_nr_processes; i+=1) {
      if(pool->worker_state[i] == BUSY)nr_busy += 1;
    }
    pthread_mutex_unlock(&pool->mutex);
    if(nr_busy==0)break;
    struct timespec ts = {0, 10000000}; // 10 ms
    nanosleep(&ts, NULL);
  }

  /* Deallocate resources associated with the processes */
  for(int i=0; i<pool->max_nr_processes; i+=1) {
    if(pool->worker[i]) {
      worker_process_free(pool->worker[i]);
      pool->worker[i] = NULL;
    }
  }
}

void process_pool_wait_and_lock(struct process_pool *pool) {

  /* Wait until a process is available */
  int err;
  do {
    err = sem_wait(&(pool->sem));
  } while(err == EINTR);
  assert(err==0);

  /* Lock the pool */
  pthread_mutex_lock(&pool->mutex);
}


void process_pool_unlock(struct process_pool *pool) {
  pthread_mutex_unlock(&pool->mutex);
}


void process_pool_cleanup(void) {

  pthread_mutex_lock(&pool_mutex);
  struct process_pool *pool = first_pool;
  while(pool) {
    struct process_pool *next = pool->next_pool;

    /* Free this process pool */
    free(pool->worker);
    free(pool->worker_state);
    pthread_mutex_destroy(&pool->mutex);
    sem_destroy(&(pool->sem));
    free(pool->executable);
    for(int i=0; i<pool->nargs; i+=1)
      free(pool->args[i]);
    free(pool->args);
    free(pool);

    /* Advance to the next pool in the list */
    pool = next;
  };
  first_pool = NULL;
  pthread_mutex_unlock(&pool_mutex);
}
