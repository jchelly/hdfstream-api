#include <stdio.h>
#include <stdint.h>
#include <msgpack.h>
#include <msgpack/fbuffer.h>
#include <hdf5.h>

#include "verify.h"
#include "pack_group.h"

/*
  Serialize a HDF5 file to msgpack by calling the hdf5/pack* functions
  directly.

  Should produce the same output as serialize_hdf5_via_lib.c
  given the same input.
*/
int main(int argc, char *argv[]) {

  (void) argc;
  verify(argc==3);
  const char *infile = argv[1];
  const char *outfile = argv[2];

  hid_t file_id = H5Fopen(infile, H5F_ACC_RDONLY, H5P_DEFAULT);
  verify(file_id>=0);

  const int buffer_size = 10*1024*1024;
  const int max_depth = 10;
  const size_t data_size_limit = SIZE_MAX;

  msgpack_packer pk;
  FILE *fd = fopen(outfile, "wb");
  msgpack_packer_init(&pk, fd, msgpack_fbuffer_write);
  verify(pack_group(file_id, pk, max_depth, data_size_limit, buffer_size) == 0);
  fclose(fd);

  H5Fclose(file_id);

  return 0;
}
