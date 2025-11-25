#include <stdlib.h>
#include <stdio.h>

#include "verify.h"

void verify_failed(char *message, char *filename, int line) {
  fprintf(stderr, "Test failed: verify(%s) at %s:%d\n", message, filename, line);
  exit(1);
}
