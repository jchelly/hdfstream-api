#define _POSIX_C_SOURCE 200809L

#include "create_test_file.h"

#include <stdlib.h>
#include <stdio.h>
#include <hdf5.h>
#include <unistd.h>

#include "verify.h"

/*
  Create an empty,  writable HDF5 file
*/
hid_t create_file(char *filename) {
  hid_t file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  verify(file_id>=0);
  return file_id;
}

/*
  Create an empty,  writable HDF5 file with a unique name.
  The template parameter is passed to mkstemp(), must be writable,
  and should end XXXXXX.

  E.g. declare with

  char template[] = "test_file_XXXXXX";
*/
hid_t create_temp_hdf5_file(char *template) {

  /* Create the file. Also updates template string. */
  int fd = mkstemp(template);
  verify(fd!=-1);

  /* Close the file descriptor */
  verify(close(fd)==0);

  /* Create a HDF5 file with the same name */
  hid_t file_id = H5Fcreate(template, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  verify(file_id>=0);

  return file_id;

}

/*
  Create an empty,  writable HDF5 file in memory
*/
hid_t create_file_in_memory(void) {

  hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
  verify(fapl_id>=0);
  H5Pset_fapl_core(fapl_id, (size_t) 1024*1024, false);
  hid_t file_id = H5Fcreate("in_mmeory.hdf5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
  verify(file_id>=0);
  return file_id;
}

/*
  Write an attribute to an object
*/
void create_attribute(hid_t obj_id, char *name, int rank, hsize_t *dims, hid_t dtype_id, void *buf) {

  /* Create dataspace */
  hid_t dspace_id = H5Screate_simple(rank, dims, NULL);

  /* Create the attribute */
  hid_t attr_id = H5Acreate(obj_id, name, dtype_id, dspace_id, H5P_DEFAULT, H5P_DEFAULT);

  /* Write data */
  H5Awrite(attr_id, dtype_id, buf);

  /* Tidy up */
  H5Aclose(attr_id);
  H5Sclose(dspace_id);
}


/*
  Create a HDF5 test dataset of the specified type
*/
void create_dataset_with_dcpl(hid_t file_id, char *datasetname, int rank, hsize_t *dims, hid_t dtype_id,
                              void (*fill_data)(int , hsize_t *, void *), hid_t dcpl_id) {

  if(dcpl_id < 0)dcpl_id = H5P_DEFAULT;

  /* Get total number of elements in the dataset */
  size_t nr_elements = 1;
  for(int i=0; i<rank; i+=1)
    nr_elements *= dims[i];

  /* Get size of the data type */
  size_t element_size = H5Tget_size(dtype_id);

  /* Allocate data buffer */
  char *data = malloc(nr_elements*element_size);
  verify(data);

  /* Call function to fill the buffer */
  fill_data(rank, dims, data);

  /* Create the dataset and write the array to it */
  hid_t dspace_id = H5Screate_simple(rank, dims, NULL);
  hid_t dataset_id = H5Dcreate(file_id, datasetname, dtype_id, dspace_id,
                               H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
  verify(dataset_id >= 0);
  herr_t err = H5Dwrite(dataset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  verify(err>=0);

  /* Tidy up */
  free(data);
  H5Sclose(dspace_id);
  H5Dclose(dataset_id);
}


FILE *create_temp_fd(void) {

  /* Open a temporary file with a unique name */
  char template[] = "test_tmp_XXXXXX";
  int fd = mkstemp(template);
  verify(fd!=-1);

  /* Immediately unlink the file */
  remove(template);

  /* Get a stdio file descriptor */
  FILE *fp = fdopen(fd, "w+");
  verify(fp);

  return fp;
}
