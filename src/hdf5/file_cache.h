#ifndef __FILE_CACHE_H
#define __FILE_CACHE_H

#include <stdlib.h>
#include <time.h>
#include <hdf5.h>

#include "ordered_map.h"
#include "dataset_cache.h"

/* Cache of open HDF5 files */
struct file_cache {
  int max_open_files;
  int max_open_datasets;
  struct ordered_map *map;
  int nr_cache_hits;
  int nr_cache_misses;
};

/* Information we store about each file */
struct file_cache_entry {
  hid_t file_id;
  time_t last_accessed;
  char *name;
  struct dataset_cache *ds_cache;
};

struct file_cache *file_cache_new(const int max_open_files, const int max_open_datasets);
struct file_cache_entry *file_cache_open_file(struct file_cache *fc, const char *name);
hid_t file_cache_open_dataset(struct file_cache_entry *file, const char *name);
void file_cache_free(struct file_cache *fc);

void file_cache_query(struct file_cache *fc, const char *file_name, const char *dataset_name, int *have_file, int *have_dataset);
void file_cache_expire_entries(struct file_cache *fc, int max_age);

#endif
