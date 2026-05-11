#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "verify.h"
#include "worker_process.h"


static void manager_process(void) {

  /* Specify executable to run */
  const char *executable = "./test_worker_failed_send";
  char *const args[] = {"test_worker_failed_send", "1", NULL};

  /* Start the process */
  struct worker_process *worker = worker_process_new(executable, args, NULL, worker_default_init, NULL);
  verify(worker);

  /* Try to send to the process. Should fail because process is no longer running. */
  int i = 1;
  sleep(1);
  int err = worker_process_send(worker, sizeof(int), &i);
  verify(err != 0);

  /* Shut down */
  worker_process_free(worker);
}


static void worker_process(void) {

  worker_init();
  abort();
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
