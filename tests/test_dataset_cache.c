#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <hdf5.h>

#include "verify.h"
#include "verify_all_closed.h"
#include "file_cache.h"


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  /* Create a new file cache */
#define MAX_OPEN_FILES 10
#define MAX_OPEN_DATASETS 10

  struct file_cache *fc = file_cache_new(MAX_OPEN_FILES, MAX_OPEN_DATASETS);

  /* Create a HDF5 file with multiple scalar datasets */
  const int nr_datasets = MAX_OPEN_DATASETS+1;
  const char filename[] = "./tmp/dataset_test.hdf5";
  char datasetname[500];
  hid_t file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  verify(file_id>=0);
  for(int i=0; i<nr_datasets; i+=1) {
    snprintf(datasetname, 500, "Dataset_%04d", i);
    hid_t dspace_id = H5Screate(H5S_SCALAR);
    hid_t dataset_id = H5Dcreate(file_id, datasetname, H5T_NATIVE_INT, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &i);
    verify(dataset_id>=0);
    H5Sclose(dspace_id);
    H5Dclose(dataset_id);
  }
  H5Fclose(file_id);

  /* Open the file */
  struct file_cache_entry *file = file_cache_open_file(fc, filename);
  verify(file);

  /* Fully populate the dataset cache */
  hid_t dataset_ids[nr_datasets];
  for(int i=0; i<MAX_OPEN_DATASETS; i+=1) {
    snprintf(datasetname, 500, "Dataset_%04d", i);
    dataset_ids[i] = file_cache_open_dataset(file, datasetname);
    verify(dataset_ids[i]>=0);
    /* Check we opened the right dataset by reading its value */
    int j;
    H5Dread(dataset_ids[i], H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &j);
    verify(i==j);
  }

  /* Opening any of these datasets should return the same dataset handle again */
  for(int i=0; i<MAX_OPEN_DATASETS; i+=1) {
    snprintf(datasetname, 500, "Dataset_%04d", i);
    hid_t dataset_id = file_cache_open_dataset(file, datasetname);
    verify(dataset_id == dataset_ids[i]);
    /* Check we opened the right dataset by reading its value */
    int j;
    H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &j);
    verify(i==j);
  }

  /* Open one more dataset so that dataset 0 should get evicted from the cache */
  snprintf(datasetname, 500, "Dataset_%04d", MAX_OPEN_DATASETS);
  hid_t dataset_id = file_cache_open_dataset(file, datasetname);
  verify(dataset_id>=0);

  /* Reopen dataset 0 and verify that we get a new dataset handle */
  snprintf(datasetname, 500, "Dataset_%04d", 0);
  dataset_id = file_cache_open_dataset(file, datasetname);
  verify(dataset_id != dataset_ids[0]);

  /* Now try opening datasets at random and making sure we get the right one */
  srand(0);
  const int nr_rep = 1000;
  for(int rep=0; rep<nr_rep; rep+=1) {
    int i = rand() % nr_datasets;
    snprintf(datasetname, 500, "Dataset_%04d", i);
    dataset_id = file_cache_open_dataset(file, datasetname);
    verify(dataset_id>=0);
    /* Check we opened the right dataset by reading its value */
    int j;
    H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &j);
    verify(i==j);
  }

  file_cache_free(fc);
  verify_all_closed();
  return 0;
}
