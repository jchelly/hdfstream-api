#include "verify_all_closed.h"
#include "verify.h"

#include <stdio.h>

static herr_t visit_id(hid_t id, void *udata) {
  verify(id>=0);
  int *count = (int *) udata;
  *count += 1;
  return 0;
}

/*
  Count identifiers which have been left open.
*/
static int count_identifiers(H5I_type_t id_type) {

  int count = 0;
  verify(H5Iiterate(id_type, visit_id, &count) == 0);
  return count;
}


/*
  Abort if HDF5 identifiers of the specified type have been left open.
*/
void verify_all_closed(void) {
  verify(count_identifiers(H5I_FILE) == 0);
  verify(count_identifiers(H5I_GROUP) == 0);
  verify(count_identifiers(H5I_DATATYPE) == 0);
  verify(count_identifiers(H5I_DATASPACE) == 0);
  verify(count_identifiers(H5I_DATASET) == 0);
  verify(count_identifiers(H5I_MAP) == 0);
  verify(count_identifiers(H5I_ATTR) == 0);
}
