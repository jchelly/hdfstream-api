#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "hdfstream.h"
#include "verify.h"

/*
  Use the libhdfstream interface to serialize a HDF5 file to msgpack

  Should produce the same output as serialize_hdf5_to_msgpack.c
  given the same input.
*/
int main(int argc, char *argv[]) {

  verify(argc==3);
  const char *infile = argv[1];
  const char *outfile = argv[2];

  /* Initialize process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(1, executable, 10, 10, 0, 0);
  verify(hs);

  /* Open the root group of the input file */
  const int max_depth = 10;
  const size_t buffer_size = 10*1024*1024;
  const size_t data_size_limit = SIZE_MAX;
  struct data_stream *stream = hdfstream_object_open(hs, infile, "/", max_depth, buffer_size, data_size_limit);
  verify(stream);

  /* Open the output file */
  FILE *fd = fopen(outfile, "wb");
  verify(fd);

  /* Allocate read buffer */
  char *buffer = malloc(buffer_size);

  /* Copy the msgpack stream to the output */
  size_t bytes_read;
  int status;
  while((bytes_read = hdfstream_read_chunk(stream, buffer, &status)))
    fwrite(buffer, sizeof(char), bytes_read, fd);
  verify(status==0);

  free(buffer);
  fclose(fd);
  hdfstream_close_stream(stream); /* Close data stream */
  hdfstream_free(hs); /* Shutdown process pool */

  return 0;
}
