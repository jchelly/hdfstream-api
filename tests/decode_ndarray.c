#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <msgpack.h>

#include "verify.h"
#include "decode_ndarray.h"

/*
  Compare a msgpack string to a C null terminated string
*/
static bool msgpack_str_equals(const msgpack_object_str obj, const char *str) {

  /* Check the lengths match */
  size_t n = strlen(str);
  if(n != obj.size)return false;

  /* Compare contents */
  return (memcmp(obj.ptr, str, n) == 0);
}

/*
  Interpret an unpacked msgpack object as an encoded ndarray.
  Returns a struct ndarray describing the array and its contents.
*/
struct ndarray decode_ndarray(msgpack_object obj) {

  struct ndarray result;
  result.status = -1;

  /* Check that this is a map representing an ndarray */
  verify(obj.type == MSGPACK_OBJECT_MAP); /* Response consists of a single msgpack map object */
  verify(obj.via.map.size == 6); /* with 6 entries */
  /* Get array of key, value pairs in the map */
  msgpack_object_kv *field = obj.via.map.ptr;
  /* First entry should be {"nd" : True} */
  verify(msgpack_str_equals(field[0].key.via.str, "nd"));
  verify(field[0].val.via.boolean);
  /* Second entry is numpy type string, e.g. {"type" : "<i4"} */
  verify(msgpack_str_equals(field[1].key.via.str, "type"));
  /* Copy the type string to the struct, noting that it is NOT null terminated in the msgpack object */
  if(field[1].val.type == MSGPACK_OBJECT_STR) {
    size_t n = field[1].val.via.str.size;
    verify(n < MAX_TYPE_LEN);
    strncpy(result.type, field[1].val.via.str.ptr, n);
    result.type[n] = '\0';
  } else {
    /* Not an atomic type, so we will not attempt to interpret it */
    result.type[0] = '\0';
  }
  /* Third entry is {"kind" : ""} */
  verify(msgpack_str_equals(field[2].key.via.str, "kind"));
  if(field[1].val.type == MSGPACK_OBJECT_STR) {
    /* Atomic types have kind='' */
    verify(field[2].val.via.str.size == 0);
  }
  /* Fourth entry is array of sizes in each dimension {"shape" : counts} */
  verify(msgpack_str_equals(field[3].key.via.str, "shape"));
  result.rank = field[3].val.via.array.size;
  for(int i=0; i<result.rank; i+=1)
    result.shape[i] = (hsize_t) field[3].val.via.array.ptr[i].via.i64;
  /* Fifth entry is total data size */
  verify(msgpack_str_equals(field[4].key.via.str, "nbytes"));
  size_t nbytes = (size_t) field[4].val.via.i64;
  /* Sixth entry contains the array data {"data" : (array of raw data buffers)} */
  verify(msgpack_str_equals(field[5].key.via.str, "data"));
  /* Get the number of buffers in the array */
  size_t nr_buffers = field[5].val.via.array.size;
  /* Compute total size of all buffers */
  result.data_len = 0;
  for(size_t buffer_nr=0; buffer_nr<nr_buffers; buffer_nr+=1)
    result.data_len += field[5].val.via.array.ptr[buffer_nr].via.bin.size;
  verify(result.data_len == nbytes);
  /* Allocate storage for the output */
  result.data = malloc(result.data_len);
  /* Copy the buffer data to the output */
  char *result_ptr = (char *) result.data;
  for(size_t buffer_nr=0; buffer_nr<nr_buffers; buffer_nr+=1) {
    const msgpack_object_bin *bin = &(field[5].val.via.array.ptr[buffer_nr].via.bin);
    memcpy(result_ptr, bin->ptr, bin->size);
    result_ptr += bin->size;
  }
  result.status = 0;
  return result;
}
