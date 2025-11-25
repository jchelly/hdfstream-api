#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
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

  /* Create a set of MAX_OPEN_FILES+1 empty HDF5 files */
  const int nr_files = MAX_OPEN_FILES+1;
  char filename[500];
  for(int i=0; i<nr_files; i+=1) {
    snprintf(filename, 500, "./tmp/test_%04d.hdf5", i);
    hid_t file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    verify(file_id>=0);
    H5Fclose(file_id);
  }

  /* Open MAX_OPEN_FILES files in sequence to fully populate the file cache */
  hid_t file_ids[MAX_OPEN_FILES];
  for(int i=0; i<MAX_OPEN_FILES; i+=1) {
    snprintf(filename, 500, "./tmp/test_%04d.hdf5", i);
    struct file_cache_entry *file = file_cache_open_file(fc, filename);
    verify(file);
    file_ids[i] = file->file_id;
  }

  /* Opening any of these files should return the same file handle again */
  for(int i=0; i<MAX_OPEN_FILES; i+=1) {
    snprintf(filename, 500, "./tmp/test_%04d.hdf5", i);
    struct file_cache_entry *file = file_cache_open_file(fc, filename);
    verify(file->file_id==file_ids[i]);
  }

  /* Open one more file so that file 0 should get evicted from the cache */
  snprintf(filename, 500, "./tmp/test_%04d.hdf5", MAX_OPEN_FILES);
  struct file_cache_entry *file = file_cache_open_file(fc, filename);

  /* Reopen file 0 and verify that we get a new file handle */
  snprintf(filename, 500, "./tmp/test_%04d.hdf5", 0);
  file = file_cache_open_file(fc, filename);
  verify(file->file_id != file_ids[0]);

  /* Now try opening files at random */
  srand(0);
  const int nr_rep = 1000;
  for(int rep=0; rep<nr_rep; rep+=1) {
    int i = rand() % nr_files;
    snprintf(filename, 500, "./tmp/test_%04d.hdf5", i);
    file = file_cache_open_file(fc, filename);
    verify(file->file_id>=0);
    /* Check that this is the expected HDF5 file */
    char check_name[500];
    ssize_t len = H5Fget_name(file->file_id, check_name, 500);
    verify(len>0);
    verify(strncmp(filename, check_name, 500)==0);
  }

  file_cache_free(fc);

  return 0;
}
