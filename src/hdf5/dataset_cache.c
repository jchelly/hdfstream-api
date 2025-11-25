#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <hdf5.h>

#include "dataset_cache.h"

/* Comparison function for dataset names */
static int cmpfunc(const void *val1, const void *val2) {
  return strcmp(val1, val2);
}

/* Allocate a new dataset cache entry */
static struct dataset_cache_entry *dataset_cache_entry_new(hid_t dataset_id, const char *name) {

  struct dataset_cache_entry *entry = malloc(sizeof(struct dataset_cache_entry));

  /* Store the file name */
  size_t len = strlen(name);
  entry->name = malloc((len+1)*sizeof(char));
  strcpy(entry->name, name);

  /* Store the dataset ID */
  entry->dataset_id = dataset_id;

  return entry;
}

/* Deallocate a cache entry */
static void dataset_cache_entry_free(struct dataset_cache_entry *entry) {
  free(entry->name);
  free(entry);
}

/* Create a new, empty cache for a file */
struct dataset_cache *dataset_cache_new(hid_t file_id, const int max_open_datasets) {

  struct dataset_cache *cache = malloc(sizeof(struct dataset_cache));
  cache->map = ordered_map_new(cmpfunc);
  cache->file_id = file_id;
  cache->max_open_datasets = max_open_datasets;
  return cache;
}

/* Deallocate a dataset cache */
void dataset_cache_free(struct dataset_cache *ds_cache) {

  /* Close all datasets and free cache entries */
  while(ordered_map_size(ds_cache->map) > 0) {
    struct dataset_cache_entry *last_entry = ordered_map_remove_tail(ds_cache->map);
#ifndef SHADOW_CACHE
    H5Dclose(last_entry->dataset_id);
#endif
    dataset_cache_entry_free(last_entry);
  }
  ordered_map_free(ds_cache->map);
  free(ds_cache);
}

/* Open a new dataset */
hid_t dataset_cache_open_dataset(struct dataset_cache *ds_cache, const char *name) {

  /* Check if this dataset is already open */
  struct dataset_cache_entry *entry = ordered_map_lookup(ds_cache->map, name);
  if(entry) {
    /* Dataset is cached. Move to start of list and return cached ID */
    assert(strcmp(entry->name, name)==0);
    ordered_map_make_item_head(ds_cache->map, name);
    return entry->dataset_id;
  }

  /* Not cached. Try to open the dataset */
#ifndef SHADOW_CACHE
  hid_t dapl_id = H5Pcreate(H5P_DATASET_ACCESS);
  const size_t chunk_cache_bytes = 64*1024*1024;
  H5Pset_chunk_cache(dapl_id, H5D_CHUNK_CACHE_NSLOTS_DEFAULT, chunk_cache_bytes, H5D_CHUNK_CACHE_W0_DEFAULT);
  hid_t dataset_id = H5Dopen(ds_cache->file_id, name, dapl_id);
  H5Pclose(dapl_id);
  if(dataset_id < 0)return dataset_id;
#else
  hid_t dataset_id = 0;
#endif

  /* If we already have the max number of entries, will need to close last dataset in list */
  assert(ds_cache->max_open_datasets>=1);
  while(ordered_map_size(ds_cache->map) >= ds_cache->max_open_datasets) {
    struct dataset_cache_entry *last_entry = ordered_map_remove_tail(ds_cache->map);
#ifndef SHADOW_CACHE
    H5Dclose(last_entry->dataset_id);
#endif
    dataset_cache_entry_free(last_entry);
  }

  /* Dataset open worked, so create a new cache entry at the head of the list */
  struct dataset_cache_entry *new_entry = dataset_cache_entry_new(dataset_id, name);
  ordered_map_add_item_head(ds_cache->map, new_entry->name, new_entry);

  return dataset_id;
}
