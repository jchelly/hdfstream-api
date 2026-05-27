#include <stdlib.h>
#include <hdf5.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>

#include "file_cache.h"
#include "ordered_map.h"


/* Comparison function for filenames */
static int cmpfunc(const void *val1, const void *val2) {
  return strcmp(val1, val2);
}


/* Allocate a new file cache entry */
static struct file_cache_entry *file_cache_entry_new(hid_t file_id, const char *name, const int max_open_datasets) {

  struct file_cache_entry *entry = malloc(sizeof(struct file_cache_entry));

  /* Store the file name */
  size_t len = strlen(name);
  entry->file_id = file_id;
  entry->name = malloc((len+1)*sizeof(char));
  strcpy(entry->name, name);

  /* Set the last access time */
  entry->last_accessed = time(NULL);

  /* Create an empty dataset cache for this file */
  entry->ds_cache = dataset_cache_new(file_id, max_open_datasets);

  return entry;
}

/* Deallocate a file cache entry */
static void file_cache_entry_free(struct file_cache_entry *entry) {
  free(entry->name);
  dataset_cache_free(entry->ds_cache);
  free(entry);
}


/* Create a new file cache */
struct file_cache *file_cache_new(const int max_open_files, const int max_open_datasets) {

  struct file_cache *fc = malloc(sizeof(struct file_cache));
  fc->max_open_files = max_open_files;
  fc->max_open_datasets = max_open_datasets;
  fc->map = ordered_map_new(cmpfunc);
  fc->nr_cache_hits = 0;
  fc->nr_cache_misses = 0;
  return fc;
}


/* Close all cached file handles and free the file cache */
void file_cache_free(struct file_cache *fc) {

  /* Close all open files */
  while(ordered_map_size(fc->map) > 0) {
    struct file_cache_entry *last_entry = ordered_map_remove_tail(fc->map);
#ifndef SHADOW_CACHE
    H5Fclose(last_entry->file_id);
#endif
    file_cache_entry_free(last_entry);
  }

  /* Free the map */
  ordered_map_free(fc->map);
  free(fc);
}


/* Open the specified file, or return an existing file handle */
struct file_cache_entry *file_cache_open_file(struct file_cache *fc, const char *name) {

  /* Check if this file is already open */
  struct file_cache_entry *entry = ordered_map_lookup(fc->map, name);
  if(entry) {
    /* File is cached. Move to start of list and return cached ID */
    assert(strcmp(entry->name, name)==0);
    ordered_map_make_item_head(fc->map, name);
    entry->last_accessed = time(NULL);
    fc->nr_cache_hits += 1;
    return entry;
  }

#ifndef SHADOW_CACHE
  /* File is not cached. Try to open it, return NULL on failure */
  hid_t prop_id = H5Pcreate(H5P_FILE_ACCESS);
#if H5_VERSION_GE(1, 14, 4)
  /* This is requried to read FLAMINGO output with "surprising" compression parameters! */
  H5Pset_relax_file_integrity_checks(prop_id, H5F_RFIC_UNUSUAL_NUM_UNUSED_NUMERIC_BITS);
#endif
#if H5_VERSION_GE(1, 10, 7)
  /* Avoid locking files if we can, since the cache holds many files open */
  H5Pset_file_locking(prop_id, false, false);
#endif
  hid_t file_id = H5Fopen(name, H5F_ACC_RDONLY, prop_id);
  H5Pclose(prop_id);
  if(file_id < 0)return NULL;
#else
  hid_t file_id = 0;
#endif

  /* If we already have the max number of entries, will need to close last file in the list */
  assert(fc->max_open_files>=1);
  while(ordered_map_size(fc->map) >= fc->max_open_files) {
    struct file_cache_entry *last_entry = ordered_map_remove_tail(fc->map);
#ifndef SHADOW_CACHE
    H5Fclose(last_entry->file_id);
#endif
    file_cache_entry_free(last_entry);
  }

  /* Create a new entry at the head of the list */
  struct file_cache_entry *new_entry = file_cache_entry_new(file_id, name, fc->max_open_datasets);
  ordered_map_add_item_head(fc->map, new_entry->name, new_entry);

  fc->nr_cache_misses += 1;
  return new_entry;
}


hid_t file_cache_open_dataset(struct file_cache_entry *file, const char *name) {
  return dataset_cache_open_dataset(file->ds_cache, name);
}

/* Query the state of the cache without modifying it */
void file_cache_query(struct file_cache *fc, const char *file_name, const char *dataset_name,
                      int *have_file, int *have_dataset) {
  assert(file_name);
  struct file_cache_entry *file = ordered_map_lookup(fc->map, file_name);
  if(file) {
    /* File is cached */
    *have_file = 1;
    /* Check if we have the dataset too */
    if(dataset_name) {
      struct dataset_cache_entry *dataset = ordered_map_lookup(file->ds_cache->map, dataset_name);
      if(dataset) {
        *have_dataset = 1;
      } else {
        *have_dataset = 0;
      }
    } else {
      /* Dataset name is NULL, i.e. not required to be in the cache */
      *have_dataset = 1;
    }
  } else {
    /* File is not cached */
    *have_file = 0;
    *have_dataset = 0;
  }
}

/* Close files which haven't been accessed in more than max_age seconds */
void file_cache_expire_entries(struct file_cache *fc, int max_age) {

  /* Non-positive max age means cache entries never expire */
  if(max_age <= 0)return;

  /* Get the current time */
  time_t now = time(NULL);

  /* Count expired cache entries */
  int nr_expired = 0;
  struct ordered_map_item *omi = NULL;
  while((omi = ordered_map_iterate(fc->map, omi))) {
    struct file_cache_entry *fce = (struct file_cache_entry *) omi->value;
    if(fce->last_accessed + (time_t) max_age < now) nr_expired += 1;
  }

  /*
    Remove the nr_expired oldest cache entries. Here we're assuming that
    they're stored in increasing order of age so we just remove the last
    nr_expired from the ordered map.
  */
  for(int i=0; i<nr_expired; i+=1) {
    struct file_cache_entry *fce = ordered_map_remove_tail(fc->map);
#ifndef SHADOW_CACHE
    H5Fclose(fce->file_id);
#endif
    file_cache_entry_free(fce);
  }

  /*
    If we just closed the last open file, try to return memory to the system
    while we're idle. Should only be done if this is a reader process.
  */
#ifndef SHADOW_CACHE
  if((nr_expired > 0) && (ordered_map_size(fc->map) == 0)) {
    malloc_trim(0);
  }
#endif
}
