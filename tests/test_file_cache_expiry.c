#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <hdf5.h>

#include "verify.h"
#include "file_cache.h"


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

#define MAX_OPEN_FILES 10
#define MAX_OPEN_DATASETS 10

  /* Create a new file cache */
  struct file_cache *fc = file_cache_new(MAX_OPEN_FILES, MAX_OPEN_DATASETS);

  /* Try to check an empty cache for expiry */
  file_cache_expire_entries(fc, 1);
  verify(ordered_map_size(fc->map) == 0);

  /* Create a file */
  char filename[] = "./tmp/test_expiry.hdf5";
  hid_t file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  H5Fclose(file_id);

  /* Use the file cache to open the file */
  struct file_cache_entry *fce = file_cache_open_file(fc, filename);
  verify(ordered_map_size(fc->map) == 1);

  /* Overwrite file cache entry timestamp to be 10s old (quicker than sleep()ing!) */
  fce->last_accessed = time(NULL) - (time_t) 10;

  /* Expire cache entries older than 1 sec */
  file_cache_expire_entries(fc, 1);

  /* Verify that the cache is now empty */
  verify(ordered_map_size(fc->map) == 0);

  /* Try to check an empty cache for expiry, again */
  file_cache_expire_entries(fc, 1);
  verify(ordered_map_size(fc->map) == 0);

  file_cache_free(fc);

  return 0;
}
