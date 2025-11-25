#ifndef CREATE_TEST_FILE_H
#define CREATE_TEST_FILE_H

#include <hdf5.h>

/*
  Create an empty,  writable HDF5 file
*/
hid_t create_file(char *filename);

/*
  Create an empty,  writable HDF5 file in memory
*/
hid_t create_file_in_memory(void);

/*
  Add an attribute to an object
*/
void create_attribute(hid_t obj_id, char *name, int rank, hsize_t *dims, hid_t dtype_id, void *buf);

/*
  Create a HDF5 test dataset of the specified type
*/
void create_dataset_with_dcpl(hid_t file_id, char *datasetname, int rank, hsize_t *dims, hid_t dtype_id,
                              void (*fill_data)(int , hsize_t *, void *), hid_t dcpl_id);
#define create_dataset(file_id, datasetname, rank, dims, dtype_id, fill_data) create_dataset_with_dcpl(file_id, datasetname, rank, dims, dtype_id, fill_data, -1)

/*
  Create (and immediately unlink) an empty, writable stdio FILE *
*/
FILE *create_temp_fd(void);

/*
  Create a temporary HDF5 file with a unique name.
  Template should end with XXXXXX and will be modified.
*/
hid_t create_temp_hdf5_file(char *template);

#endif
