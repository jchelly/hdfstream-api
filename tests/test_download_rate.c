#include <stdlib.h>
#include <stdio.h>

#include "verify.h"
#include "hdfstream.h"
#include "receive_ndarray.h"


/*
  Read a dataset repeatedly via the C API and report the average time taken
*/
int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;
  
  /* Initialize process pool */
  char *executable = "../src/reader/hdfstream_reader";
  struct hdfstream *hs = hdfstream_new_with_executable(1, executable, 10, 10, 0, 0);
  if(!hs) {
    fprintf(stderr, "Failed to initialize hdfstream!\n");
    exit(1);
  }

  /* Specify file and dataset to read */
  char filename[] = "/cosma7/data/Eagle/DataRelease/L0100N1504/PE/DMONLY/data/snapshot_028_z000p000/snap_028_z000p000.0.hdf5";
  char datasetname[] = "PartType1/Coordinates";

  /* Specify dataset elements to read */
  int rank = 2;
  hsize_t start[] = {0,0};
  hsize_t count[] = {27100595,3};

  const int nr_reps = 10;
  const size_t buffer_size = 32*1024*1024;

  for(int rep_nr=0; rep_nr<nr_reps; rep_nr+=1) {

    printf("Iteration %d\n", rep_nr);

    /* Read the dataset*/
    struct ndarray res = receive_ndarray(hs, filename, datasetname, rank, start, count, buffer_size);
    verify(res.status==0);

    /* Free the data*/
    free(res.data);
  }

  /* Shut down */
  hdfstream_free(hs);
}
