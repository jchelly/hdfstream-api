#ifndef __WORKER_PROCESS_H
#define __WORKER_PROCESS_H

#define _POSIX_C_SOURCE 200809L
#include <semaphore.h>

struct worker_process;

/* Function pointer type used for initialization and shutdown callbacks */
typedef int (*worker_callback)(struct worker_process *worker);

struct worker_process {
  /* Process ID */
  int pid;
  /* Pipe to send input to child process */
  int input_pipe[2];
  /* Pipe to receive output from child process*/
  int output_pipe[2];
  /* User data pointer */
  void *data;
  /* Callbacks */
  worker_callback init;
  worker_callback shutdown;
  /* Flag if read/write caused EPIPE, which indicates a dead process */
  int is_dead;
  /* Exit status, set if/when the process dies */
  int exit_status;
};

/*
   Default initialization callback which expects to receive a single int from
   the child process to confirm startup. The child can use worker_init() to do
   this.
*/
int worker_default_init(struct worker_process *worker);

/*
  Start up a new worker process. Returns NULL on failure.

  executable: string with the name of the executable to run
  args: array of arguments for the executable
  data: may be used to pass in information for initialization
  init_callback: called when the new process starts up
  shutdown_callback: called to clean up when the process exists

  Callbacks should return 0 on success, non-zero on failure.
*/
struct worker_process *worker_process_new(const char *executable, char *const args[], void *data,
                                          worker_callback init_callback, worker_callback shutdown_callback);

/* Check a process is still running */
int worker_process_is_alive(struct worker_process *worker);

/* Terminate and deallocate a worker process and return its exit code */
int worker_process_free(struct worker_process *worker);

/* Functions for communication with the worker process, returns 0 on success, -1 on failure */
int worker_process_send(struct worker_process *worker, const size_t len, const void *data);
int worker_process_recv(struct worker_process *worker, const size_t len, void *data);
int worker_process_wait_for_semaphore(struct worker_process *worker, sem_t *s, int delay);

/* Function for worker process to confirm successful startup */
void worker_init(void);

/* Functions for worker process to communicate with manager */
int worker_send(const size_t len, const void *data);
int worker_recv(const size_t len, void *data);
int worker_recv_with_timeout(const size_t len, void *data, int timeout);

#endif
