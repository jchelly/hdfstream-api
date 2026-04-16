#include <msgpack.h>

#include "pack_unknown.h"
#include "verify.h"

int pack_unknown(msgpack_packer pk) {

  int result = -1;

  /* Make a msgpack map with one entry */
  check(msgpack_pack_map(&pk, 1));

  /* Add entry to identify this as an unknown type */
  check(msgpack_pack_str(&pk, 11));
  check(msgpack_pack_str_body(&pk, "hdf5_object", 11));
  check(msgpack_pack_str(&pk, 7));
  check(msgpack_pack_str_body(&pk, "unknown", 7));

  /* Success */
  result = 0;

 cleanup:
  return result;
}
