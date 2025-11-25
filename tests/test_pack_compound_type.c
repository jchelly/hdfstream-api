#include <stdio.h>
#include <msgpack.h>
#include <hdf5.h>
#include <string.h>

#include "verify.h"
#include "../src/hdf5/pack_numpy_type.h"


int main(int argc, char *argv[]) {

  (void) argc;
  (void) argv;

  struct compound_t {
    int a;
    int b;
  };
  const size_t size = sizeof(struct compound_t);

  /* Create a HDF5 compound data type matching the struct */
  hid_t dtype_id = H5Tcreate(H5T_COMPOUND, size);
  H5Tinsert(dtype_id, "a", offsetof(struct compound_t, a), H5T_NATIVE_INT);
  H5Tinsert(dtype_id, "b", offsetof(struct compound_t, b), H5T_NATIVE_INT);

  /* Pack the datatype to a msgpack buffer */
  msgpack_sbuffer sbuf;
  msgpack_packer pk;
  msgpack_sbuffer_init(&sbuf);
  msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
  verify(pack_numpy_type(dtype_id, NULL, &pk)==0);

  /*
    Now try to unpack the type description. It should be something
    like

    [["a", "<i4"], ["b", "<i4"]]

    depending on the size and endian-ness of native ints.
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

  /* Result should be an array containing two sub arrays */
  verify(obj.type==MSGPACK_OBJECT_ARRAY);
  verify(obj.via.array.size == 2);

  /* Check first field ["a", "<i4"] */
  msgpack_object *field0 = obj.via.array.ptr+0;
  verify(field0->type==MSGPACK_OBJECT_ARRAY);
  verify(field0->via.array.size==2);
  msgpack_object *name0 = field0->via.array.ptr+0;
  msgpack_object *type0 = field0->via.array.ptr+1;
  verify(name0->type==MSGPACK_OBJECT_STR);
  verify(name0->via.str.size==1);
  verify(strncmp(name0->via.str.ptr, "a", name0->via.str.size)==0);
  verify(type0->type==MSGPACK_OBJECT_STR);
  verify(type0->via.str.size==3);
  verify(type0->via.str.ptr[1]=='i');

  /* Check second field ["b", "<i4"] */
  msgpack_object *field1 = obj.via.array.ptr+1;
  verify(field1->type==MSGPACK_OBJECT_ARRAY);
  verify(field1->via.array.size==2);
  msgpack_object *name1 = field1->via.array.ptr+0;
  msgpack_object *type1 = field1->via.array.ptr+1;
  verify(name1->type==MSGPACK_OBJECT_STR);
  verify(name1->via.str.size==1);
  verify(strncmp(name1->via.str.ptr, "b", name1->via.str.size)==0);
  verify(type1->type==MSGPACK_OBJECT_STR);
  verify(type1->via.str.size==3);
  verify(type1->via.str.ptr[1]=='i');

  /* Check we don't have any leftover data */
  verify(unp.off == unp.used);

  /* Tidy up */
  msgpack_unpacked_destroy(&und);
  msgpack_unpacker_destroy(&unp);
  msgpack_sbuffer_destroy(&sbuf);
  H5Tclose(dtype_id);

  return 0;
}
