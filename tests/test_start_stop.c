#include <stdlib.h>
#include <stdio.h>
#include <msgpack.h>

#include "verify.h"
#include "hdfstream.h"


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  /* Initialize process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(1, executable, 10, 10, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

  /* Shut down */
  hdfstream_free(hs);

  return 0;
}
