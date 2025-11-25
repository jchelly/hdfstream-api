#include <stdio.h>
#include <msgpack.h>
#include <hdf5.h>
#include <string.h>

#include "verify.h"
#include "../src/hdf5/pack_numpy_type.h"


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  /* Create a HDF5 array data type */
  const int rank = 2;
  const hsize_t dims[] = {3,3};
  hid_t dtype_id = H5Tarray_create(H5T_NATIVE_FLOAT, rank, dims);

  /* Pack the datatype to a msgpack buffer */
  msgpack_sbuffer sbuf;
  msgpack_packer pk;
  msgpack_sbuffer_init(&sbuf);
  msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
  verify(pack_numpy_type(dtype_id, NULL, &pk)==0);

  /*
    Now try to unpack the type description. It should be something
    like

    ["<f4", (3,3)].

    depending on the size and endian-ness of native floats.
  */

  /* Initialize unpacker and feed it the serialized data */
  msgpack_unpacker unp;
  msgpack_unpacker_init(&unp, sbuf.size);
  msgpack_unpacker_reserve_buffer(&unp, sbuf.size);
  memcpy(msgpack_unpacker_buffer(&unp), sbuf.data, sbuf.size);
  msgpack_unpacker_buffer_consumed(&unp, sbuf.size);

  /* Unpack and check the output */
  msgpack_unpacked und;
  msgpack_unpacked_init(&und);
  verify(msgpack_unpacker_next(&unp, &und) == MSGPACK_UNPACK_SUCCESS);
  msgpack_object obj = und.data;

  /* Result should be an array containing a string and an array */
  verify(obj.type==MSGPACK_OBJECT_ARRAY);
  verify(obj.via.array.size == 2);

  /* First element is type string */
  msgpack_object *typestr = obj.via.array.ptr+0;
  verify(typestr->type==MSGPACK_OBJECT_STR);
  verify(typestr->via.str.size==3);
  verify(typestr->via.str.ptr[1]=='f');

  /* Second element is an array with the shape */
  msgpack_object *shape = obj.via.array.ptr+1;
  verify(shape->type==MSGPACK_OBJECT_ARRAY);
  verify(shape->via.array.size==2);
  for(int i=0; i<2; i+=1) {
    verify(shape->via.array.ptr[i].type == MSGPACK_OBJECT_POSITIVE_INTEGER);
    verify(shape->via.array.ptr[i].via.i64 == 3);
  }

  /* Check we don't have any leftover data */
  verify(unp.off == unp.used);

  /* Tidy up */
  msgpack_unpacked_destroy(&und);
  msgpack_unpacker_destroy(&unp);
  msgpack_sbuffer_destroy(&sbuf);
  H5Tclose(dtype_id);

  return 0;
}
