#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "verify.h"
#include "worker_process.h"


static void manager_process(void) {

  /* Deliberately wrong to check we can detect a failed execv()  */
  char *executable = "./no-such-file";
  char *args[] = {"no-such-file", "1", NULL};

  /* Start the process */
  struct worker_process *worker = worker_process_new(executable, args, NULL, worker_default_init, NULL);
  verify(worker==NULL);

}

static void worker_process(void) {

  worker_init();
  while(1) {};
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
