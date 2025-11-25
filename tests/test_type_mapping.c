#include <hdf5.h>

#include "verify.h"
#include "type_mapping.h"

int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  const size_t len = 100;
  char descr[len];

  int err;

  err = numpy_type_info(H5T_NATIVE_INT, len, descr);
  verify(err==0);
  verify((descr[0]=='>') || (descr[0]=='<'));
  verify(descr[1]=='i');
  verify(descr[2]=='4');

  err = numpy_type_info(H5T_NATIVE_UINT, len, descr);
  verify(err==0);
  verify((descr[0]=='>') || (descr[0]=='<'));
  verify(descr[1]=='u');
  verify(descr[2]=='4');

  err = numpy_type_info(H5T_NATIVE_LLONG, len, descr);
  verify(err==0);
  verify((descr[0]=='>') || (descr[0]=='<'));
  verify(descr[1]=='i');
  verify(descr[2]=='8');

  err = numpy_type_info(H5T_NATIVE_ULLONG, len, descr);
  verify(err==0);
  verify((descr[0]=='>') || (descr[0]=='<'));
  verify(descr[1]=='u');
  verify(descr[2]=='8');

  err = numpy_type_info(H5T_NATIVE_FLOAT, len, descr);
  verify(err==0);
  verify((descr[0]=='>') || (descr[0]=='<'));
  verify(descr[1]=='f');
  verify(descr[2]=='4');

  err = numpy_type_info(H5T_NATIVE_DOUBLE, len, descr);
  verify(err==0);
  verify((descr[0]=='>') || (descr[0]=='<'));
  verify(descr[1]=='f');
  verify(descr[2]=='8');

  /* Try a fixed length string */
  const int str_len = 9;
  hid_t str_type = H5Tcreate(H5T_STRING, str_len);
  err = numpy_type_info(str_type, len, descr);
  verify(err==0);
  verify(descr[0]=='S');
  verify(descr[1]=='9');
  H5Tclose(str_type);

  return 0;
}
