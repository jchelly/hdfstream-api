#ifndef __DATASET_CACHE_H
#define __DATASET_CACHE_H

#include <stdlib.h>
#include <hdf5.h>

#include "ordered_map.h"

/* Dataset cache info for one open file */
struct dataset_cache {
  int max_open_datasets;
  hid_t file_id;             /* Associated HDF5 file ID */
  struct ordered_map *map; /* Ordered map of dataset_cache_entry structs */
};

/* Information we store about each dataset */
struct dataset_cache_entry {
  hid_t dataset_id;
  char *name;
};

/* Create a new, empty cache for a file */
struct dataset_cache *dataset_cache_new(hid_t file_id, const int max_open_datasets);

/* Open a new dataset */
hid_t dataset_cache_open_dataset(struct dataset_cache *ds_cache, const char *name);

/* Deallocate a cache */
void dataset_cache_free(struct dataset_cache *ds_cache);

#endif
