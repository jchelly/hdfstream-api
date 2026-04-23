#include "verify.h"
#include "verify_all_closed.h"
#include "select.h"

#include <stdlib.h>
#include <hdf5.h>


static void test_select_slices_recursive(const int n) {

  /* Create a dataspace with n elements */
  const int rank = 1;
  hsize_t dims[] = {n};
  hid_t dspace_id = H5Screate_simple(rank, dims, NULL);

  /* Create an array of slices to select */
  const int nr_slices = n / 2;
  hsize_t *select_start = malloc(sizeof(hsize_t)*nr_slices);
  hsize_t *select_count = malloc(sizeof(hsize_t)*nr_slices);
  for(int i=0; i<nr_slices; i+=1) {
    verify(2*i+1 <= n);
    select_start[i] = 2*i+0;
    select_count[i] = 1;
  }
  hsize_t start[1] = {0};
  hsize_t count[1] = {1};

  /* Select the slices */
  verify(select_slices(dspace_id, nr_slices, select_start, select_count, start, count) == 0);
  verify(H5Sget_select_npoints(dspace_id) == nr_slices);

  H5Sclose(dspace_id);
  verify_all_closed();

  free(select_start);
  free(select_count);
}


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  test_select_slices_recursive(10000);

  return 0;
}
