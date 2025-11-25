#include <stdlib.h>
#include <stdio.h>
#include <msgpack.h>

#include "verify.h"
#include "hdfstream.h"
#include "create_test_file.h"
#include "receive_ndarray.h"


static void fill_data(int rank, hsize_t *dims, void *data) {

  int *ptr = (int *) data;

  size_t nr_elements = 1;
  for(int i=0; i<rank; i+=1)
    nr_elements *= dims[i];

  for(size_t i=0; i<nr_elements; i+=1)
    ptr[i] = (int) i;
}

/*
  Here we construct a corrupted HDF5 file to check that we can recover from
  a failed read which happens in the middle of serialization.

  A read failure is ensured by enabling the checksum filter then overwriting
  part of a data chunk.
*/

int main(int argc, char *argv[]) {

  (void) argc;

  hsize_t total_size = 10000;
  hsize_t chunk_size = 100; // 100 chunks total
  hsize_t chunk_idx = 40; // Chunk 40 will be corrupted

  /* Set up the dataset creation property list */
  hid_t dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
  hsize_t dim[] = {chunk_size};
  H5Pset_chunk(dcpl_id, 1, dim);
  H5Pset_fletcher32(dcpl_id);

  /* Create a test dataset */
  char *filename = argv[1];
  hid_t file_id = create_file(filename);
  char datasetname[] = "test_dataset";
  int rank = 1;
  hsize_t dims[] = {total_size};
  create_dataset_with_dcpl(file_id, datasetname, rank, dims, H5T_NATIVE_INT, fill_data, dcpl_id);
  /* Get the offset into the file of a data chunk */
  hid_t dset_id = H5Dopen(file_id, datasetname, H5P_DEFAULT);
  verify(dset_id >= 0);
  haddr_t chunk_file_offset;
  hsize_t chunk_file_size;
  hid_t dspace_id = H5Dget_space(dset_id);
  verify(H5Dget_chunk_info(dset_id, dspace_id, chunk_idx, NULL, NULL, &chunk_file_offset, &chunk_file_size) >= 0);
  H5Sclose(dspace_id);
  H5Dclose(dset_id);
  H5Fclose(file_id);
  H5Pclose(dcpl_id);

  /* Now sabotage the file! */
  FILE *fd = fopen(filename, "rb+");
  verify(fd);
  fseek(fd, (long int) chunk_file_offset, SEEK_SET);
  int dummy = 0;
  fwrite(&dummy, sizeof(int), 1, fd);
  fclose(fd);

  /* Initialize process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(1, executable, 10, 10, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

  /* Check that we can correctly read the non-corrupt part of the dataset */
  hsize_t start[] = {0};
  hsize_t count[] = {chunk_idx*chunk_size};
  size_t buffer_size = 150; // smaller than the non-corrupt part
  struct ndarray res = receive_ndarray(hs, filename, datasetname, rank, start, count, buffer_size);
  verify(res.status==0);
  verify(res.rank==1);
  verify(res.shape[0] == count[0]);
  verify(res.type[1] == 'i');
  verify(res.data_len == count[0]*sizeof(int));
  int *ptr = (int *) res.data;
  for(hsize_t i=0; i<count[0]; i+=1)
    verify((hsize_t) ptr[i] == i);
  free(res.data);

  /* Now try to read the full dataset, which should fail */
  count[0] = dims[0];
  res = receive_ndarray(hs, filename, datasetname, rank, start, count, buffer_size);
  verify(res.status!=0);

  /* Then make sure we're still able to read the correct part */
  count[0] = chunk_idx*chunk_size;
  res = receive_ndarray(hs, filename, datasetname, rank, start, count, buffer_size);
  verify(res.status==0);
  verify(res.rank==1);
  verify(res.shape[0] == count[0]);
  verify(res.type[1] == 'i');
  verify(res.data_len == count[0]*sizeof(int));
  ptr = (int *) res.data;
  for(hsize_t i=0; i<count[0]; i+=1)
    verify((hsize_t) ptr[i] == i);
  free(res.data);

  hdfstream_free(hs);

  return 0;
}
