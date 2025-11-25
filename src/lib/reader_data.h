#include "hdfstream.h"
#include "worker_process.h"
#include "shared_memory.h"

/* Data stored on the parent process for each reader process */
struct reader_data {
  struct file_cache *file_cache;
  struct shared_memory *sm;
};

/* Callback called when a new reader process is created */
int reader_init(struct worker_process *worker);

/* Callback called when a reader process is shut down */
int reader_shutdown(struct worker_process *worker);
