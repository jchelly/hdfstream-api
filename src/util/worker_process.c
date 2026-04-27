#define _POSIX_C_SOURCE 200809L
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>

#include "worker_process.h"

#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

/*
  write() is not guaranteed to write all the data passed to it.
  This keeps calling write() until all bytes have been written.

  Returns -1 on failure (or -2 if pipe is broken).
*/
static ssize_t do_write(int filedes, const void *buf, size_t nbytes, int block_sigpipe) {

  size_t nleft = nbytes;
  char *ptr = (char *) buf;

  sigset_t old_set, new_set;
  if(block_sigpipe) {
    /* Temporarily block SIGPIPE so we don't die if the pipe is broken.
       First we need to get the current signal mask. */
    sigemptyset(&old_set);
    pthread_sigmask(SIG_SETMASK, NULL, &old_set);
    /* Then block SIGPIPE */
    sigemptyset(&new_set);
    sigaddset(&new_set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &new_set, NULL);
  }
  /* And write the data */
  while(nleft > 0) {
    ssize_t nwritten = write(filedes, ptr, nleft);
    if(nwritten < 0) {
      if(errno == EINTR)continue;
      if(errno == EPIPE) {
        /* Broken pipe error */
        if(block_sigpipe) {
          /* Consume the sigpipe before unblocking it */
          siginfo_t info;
          struct timespec timeout = {0, 0};
          sigtimedwait(&new_set, &info, &timeout);
        }
        nbytes = -2;
        break;
      } else {
        /* Some other error */
        nbytes = -1;
        break;
      }
    } else {
      ptr += nwritten;
      nleft -= nwritten;
    }
  }
  if(block_sigpipe) {
    /* Then restore the signal mask */
    pthread_sigmask(SIG_SETMASK, &old_set, NULL);
  }

  return (ssize_t) nbytes;
}


/*
  read() is not guaranteed to read all the data passed to it.
  This keeps calling read() until all bytes have been read.

  Returns number of bytes read or negative number on failure:

  -3: end of file
  -2: broken pipe
  -1: another error
*/
static ssize_t do_read(int filedes, void const *buf, size_t nbytes, int block_sigpipe) {

  size_t nleft = nbytes;
  char *ptr = (char *) buf;

  sigset_t old_set, new_set;
  if(block_sigpipe) {
    /* Temporarily block SIGPIPE so we don't die if the pipe is broken.
       First we need to get the current signal mask. */
    sigemptyset(&old_set);
    pthread_sigmask(SIG_SETMASK, NULL, &old_set);
    /* Then block SIGPIPE */
    sigemptyset(&new_set);
    sigaddset(&new_set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &new_set, NULL);
  }
  /* And read the data */
  while(nleft > 0) {
    ssize_t nread = read(filedes, ptr, nleft);
    if(nread == 0) {
      /* End of file */
      nbytes = -3;
      break;
    } else if(nread < 0) {
      if(errno == EINTR)continue;
      if(errno == EPIPE) {
        /* Broken pipe error */
        if(block_sigpipe) {
          /* Consume the sigpipe before unblocking it */
          siginfo_t info;
          struct timespec timeout = {0, 0};
          sigtimedwait(&new_set, &info, &timeout);
        }
        nbytes = -2;
        break;
      } else {
        /* Some other error */
        nbytes = -1;
        break;
      }
    } else {
      ptr += nread;
      nleft -= nread;
    }
  }
  if(block_sigpipe) {
    /* Then restore the signal mask */
    pthread_sigmask(SIG_SETMASK, &old_set, NULL);
  }

  return (ssize_t) nbytes;
}


/* Start up a new worker process, return NULL on failure */
struct worker_process *worker_process_new(const char *executable, char *const args[],
                                          void *data, worker_callback init_callback,
                                          worker_callback shutdown_callback) {

  struct worker_process *worker = malloc(sizeof(struct worker_process));
  worker->data = data;
  worker->init = init_callback;
  worker->shutdown = shutdown_callback;
  worker->is_dead = 0;
  worker->exit_status = 0;

  /* Create the pipes to communicate with the child process */
  if (pipe(worker->input_pipe) == -1) {
    free(worker);
    return NULL;
  }
  if (pipe(worker->output_pipe) == -1) {
    free(worker);
    return NULL;
  }

  /* Fork the new process */
  worker->pid = fork();
  if (worker->pid == -1) {

    /* Only get here if fork() failed */
    free(worker);
    return NULL;

  } else if (worker->pid == 0) {

    /* Child will not write to input pipe or read from output pipe */
    close(worker->input_pipe[PIPE_WRITE]);
    close(worker->output_pipe[PIPE_READ]);

    /* Route child's stdout to the output pipe and child's stdin to the input pipe */
    while ((dup2(worker->output_pipe[PIPE_WRITE], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
    while ((dup2(worker->input_pipe[PIPE_READ], STDIN_FILENO) == -1) && (errno == EINTR)) {}
    close(worker->input_pipe[PIPE_READ]);
    close(worker->output_pipe[PIPE_WRITE]);

    /* Run the executable */
    execv(executable, args);

    /* If exec fails, abort the newly forked process */
    abort();

  } else {
    /* Parent will not read child's input or write to its output */
    close(worker->input_pipe[PIPE_READ]);
    close(worker->output_pipe[PIPE_WRITE]);
  }

  /* Call the init callback, if set. Ideally this should check that the process is running. */
  if(worker->init) {
    if(worker->init(worker) != 0) {
      /* Something went wrong so clean up and return null */
      worker_process_free(worker);
      return NULL;
    }
  }

  return worker;
}


/*
  Wait for worker to terminate and then deallocate it.
  Sends a SIGTERM if the process hasn't already terminated.
*/
int worker_process_free(struct worker_process *worker) {

  /* Wait for child process to complete and check return code */
  if(!worker->is_dead) {
    kill(worker->pid, SIGTERM);

    // Wait up to 200ms for graceful exit
    int waited = 0;
    int status;
    while (waited < 200) {
      pid_t r = waitpid(worker->pid, &status, WNOHANG);
      if (r == worker->pid) {
        worker->exit_status = status;
        goto cleanup;
      }
      struct timespec ts = {0, 1000000}; // 1 ms
      nanosleep(&ts, NULL);
      waited+=1;
    }

    // Still alive, so force kill it
    kill(worker->pid, SIGKILL);
    waitpid(worker->pid, &worker->exit_status, 0);
  }

 cleanup:

  /* Close stdin/stdout file descriptors */
  close(worker->input_pipe[PIPE_WRITE]);
  close(worker->output_pipe[PIPE_READ]);

  /* Clean up */
  if(worker->shutdown)worker->shutdown(worker);
  int status = worker->exit_status;
  free(worker);

  /* Return worker process return code */
  return status;
}


/*
  Send kill signal to a process
*/
void worker_process_kill(struct worker_process *worker) {
  if(!worker->is_dead)kill(worker->pid, SIGKILL);
}

/*
   Send a message to the worker process stdin, return 0 on success, -1 on failure.
   Will flag the worker process as dead if we get an EPIPE.
*/
int worker_process_send(struct worker_process *worker, const size_t len, const void *data) {
  if(worker->is_dead)return -1;
  ssize_t nr = do_write(worker->input_pipe[PIPE_WRITE], data, len, 1);
  if(nr == -2) {
    worker->is_dead = 1;
    waitpid(worker->pid, &worker->exit_status, 0);
  }
  if((nr < 0) || (((size_t) nr) != len))
    return -1;
  else
    return 0;
}


/*
  Receive a message from the worker process stdout, return 0 on success, -1 on failure.
  Will flag the worker process as dead if we get a broken pipe or end of file.
*/
int worker_process_recv(struct worker_process *worker, const size_t len, void *data) {
  if(worker->is_dead)return -1;
  ssize_t nr = do_read(worker->output_pipe[PIPE_READ], data, len, 1);
  if((nr == -2) || (nr == -3)){
    worker->is_dead = 1;
    waitpid(worker->pid, &worker->exit_status, 0);
  }
  if((nr < 0) || (((size_t) nr) != len))
    return -1;
  else
    return 0;
}

/*
  Check if a worker process is alive
*/
int worker_process_is_alive(struct worker_process *worker) {
  if(worker->is_dead) {
    /* Already died, so nothing to do */
    return 0;
  } else {
    /* Check the state of the process */
    if(waitpid(worker->pid, &worker->exit_status, WNOHANG) == worker->pid) {
      /* Process has died */
      worker->is_dead = 1;
      return 0;
    } else {
      /* Process is still alive */
      return 1;
    }
  }
}

/* Function for worker process to send to manager, return 0 on success, -1 on failure */
int worker_send(const size_t len, const void *data) {
  ssize_t nr = do_write(STDOUT_FILENO, data, len, 0);
  if((nr < 0) || (((size_t) nr) != len))
    return -1;
  else
    return 0;
}

/* Function for worker process to receive from manager, return 0 on success, -1 on failure */
int worker_recv(const size_t len, void *data) {
  ssize_t nr = do_read(STDIN_FILENO, data, len, 0);
  if((nr < 0) || (((size_t) nr) != len))
    return -1;
  else
    return 0;
}

/* Function for worker process to confirm that it has started up successfully */
void worker_init(void) {
  int i = 1;
  worker_send(sizeof(int), &i);
}

/*
   Function for worker process to receive from manager with a timeout.

   Returns
   0 - success
  -1 - failure
   1 - timeout reached

   Timeout here is in milliseconds.
*/
int worker_recv_with_timeout(const size_t len, void *data, int timeout) {

  struct pollfd fds;
  fds.fd = STDIN_FILENO;
  fds.events = POLLIN;

  int result;
  do {
    result = poll(&fds, (nfds_t) 1, timeout);
  } while ((result < 0) && (errno==EINTR));

  if(result < 0) {
    /* Something went wrong */
    return -1;
  } else if(result==0) {
    /* Timed out */
    return 1;
  } else {
    /* There is data to read */
    return worker_recv(len, data);
  }
}

/*
  Default worker init function.

  This assumes that the called executable is going to worker_send() a single
  int to confirm startup.
*/
int worker_default_init(struct worker_process *worker) {

  int dummy = 0;
  if(worker_process_recv(worker, sizeof(int), &dummy) < 0) {
    return -1;
  } else {
    return 0;
  }
}

/*
  Wait for the specified shared semaphore, assuming that the worker
  will sem_post() it at some point if necessary.

  Returns 0 on success.

  Returns -1 if the worker process died and therefore the semaphore
  will never become available.

  Checks the state of the worker every delay seconds.
*/
int worker_process_wait_for_semaphore(struct worker_process *worker, sem_t *s, int delay) {

  while(1) {

    /* Compute deadline for semaphore timeout */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += delay;

    /* Wait for the semaphore */
    int status;
    while ((status = sem_timedwait(s, &ts)) == -1 && errno == EINTR)
      continue;

    /* Check if we timed out */
    if(status == 0) {
      /* Success */
      return 0;
    } else {
      /* Timed out, so check state of worker process */
      if(!worker_process_is_alive(worker))return -1;
    }
  }
}
