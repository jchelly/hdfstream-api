#include <msgpack.h>
#include <hdf5.h>

#include "pack_and_decode.h"
#include "decoder.h"
#include "pack_object.h"
#include "verify.h"

/*
  Given a HDF5 object identifier, serialize it to an in-memory buffer
  and then decode it.
*/
hs_object pack_and_decode(hid_t obj_id, int max_depth,  size_t data_size_limit, size_t buffer_size) {

  /* Initalize packer to pack to a memory buffer  */
  msgpack_sbuffer sbuf;
  msgpack_sbuffer_init(&sbuf);
  msgpack_packer pk;
  msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

  /* Serialize the HDF5 object */
  verify(pack_object(obj_id, ".", pk, max_depth, data_size_limit, buffer_size) == 0);

  /* Interpret the packed data */
  msgpack_unpacked result;
  msgpack_unpacked_init(&result);
  verify(msgpack_unpack_next(&result, sbuf.data, sbuf.size, NULL));
  hs_object root = hs_decode_object(result.data);
  verify(root.type != HS_NULL);

  /* Tidy up */
  msgpack_unpacked_destroy(&result);
  msgpack_sbuffer_destroy(&sbuf);

  return root;
}
