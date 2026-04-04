#include "decoder.h"
#include "verify.h"
#include <msgpack.h>

/* Compare a msgpack string to a C null terminated string */
static bool msgpack_str_equals(const msgpack_object_str obj, const char *str) {
  size_t n = strlen(str);
  if(n != obj.size)return false;
  return (memcmp(obj.ptr, str, n) == 0);
}

static char *copy_string(msgpack_object obj) {
  verify(obj.type == MSGPACK_OBJECT_STR);
  size_t len = obj.via.str.size;
  char *result = malloc(sizeof(char)*(len+1));
  strncpy(result, obj.via.str.ptr, len);
  result[len] = '\0';
  return result;
}

static char **copy_map_keys(msgpack_object obj) {
  verify(obj.type == MSGPACK_OBJECT_MAP);
  size_t nr_keys = obj.via.map.size;
  char **buf = malloc(sizeof(char *)*nr_keys);
  msgpack_object_kv *field = obj.via.map.ptr;
  for(size_t i=0; i<nr_keys; i+=1)
    buf[i] = copy_string(field[i].key);    
  return buf;
}

static hs_attrs decode_attributes(msgpack_object obj) {
  verify(obj.type == MSGPACK_OBJECT_MAP);
  hs_attrs result;
  result.nr_attrs = obj.via.map.size;
  result.name = copy_map_keys(obj);
  size_t nr_keys = obj.via.map.size;
  result.value = malloc(nr_keys*sizeof(hs_object));
  for(size_t i=0; i<nr_keys; i+=1)
    result.value[i] = hs_decode_object(obj.via.map.ptr[i].val);
  return result;  
}

static hs_object decode_group(msgpack_object obj) {

  hs_object result;
  result.type = HS_ERROR;

  /* First entry identifies this as a group */
  verify(obj.type == MSGPACK_OBJECT_MAP);
  verify(obj.via.map.size > 0);
  msgpack_object_kv *field = obj.via.map.ptr;
  verify(field[0].key.type == MSGPACK_OBJECT_STR);
  verify(field[0].val.type == MSGPACK_OBJECT_STR);
  verify(msgpack_str_equals(field[0].key.via.str, "hdf5_object"));
  verify(msgpack_str_equals(field[0].val.via.str, "group"));

  /* Initialize the output object */
  result.type = HS_GROUP;
  result.group.nr_members = -1;
  result.group.member_name = NULL;
  result.group.member_object = NULL;
  result.group.attrs.name = NULL;
  result.group.attrs.value = NULL;
	
  /* Check for empty place-holder */
  if(obj.via.map.size == 1)return result;

  /* Otherwise, we should have members and attributes fields */
  verify(obj.via.map.size == 3);
  for(size_t i=0; i<obj.via.map.size; i+=1) {
    verify(field[i].key.type == MSGPACK_OBJECT_STR);
    if(msgpack_str_equals(field[i].key.via.str, "members")) {
      verify(field[i].val.type == MSGPACK_OBJECT_MAP);
      /* Store number of group members */
      result.group.nr_members = field[i].val.via.map.size;
      /* Store member names */
      result.group.member_name = copy_map_keys(field[i].val);
      /* Store member objects */
      result.group.member_object = malloc(result.group.nr_members*sizeof(hs_object));
      for(int member_nr=0; member_nr<result.group.nr_members; member_nr+=1) {
	result.group.member_object[member_nr] = hs_decode_object(field[i].val.via.map.ptr[member_nr].val);
      }
    } else if(msgpack_str_equals(field[i].key.via.str, "attributes")) {
      /* Decode the group's attributes */
      result.group.attrs = decode_attributes(field[i].val);
    } else if(msgpack_str_equals(field[i].key.via.str, "hdf5_object")) {
      /* Nothing to do */
    } else {
      /* Unrecognized field */
      hs_free_object(result);
      result.type = HS_ERROR;
      return result;
    }
  }
  return result;
}

static size_t *decode_shape(msgpack_object obj) {
  verify(obj.type == MSGPACK_OBJECT_ARRAY);
  size_t n = obj.via.array.size;
  size_t *shape = malloc(sizeof(size_t)*n);
  for(size_t i=0; i<n; i+=1)
    shape[i] = obj.via.array.ptr[i].via.i64;
  return shape;
}

static hs_object decode_dataset(msgpack_object obj) {

  hs_object result;
  result.type = HS_ERROR;

  verify(obj.type == MSGPACK_OBJECT_MAP);

  /* First entry identifies this as a dataset */
  verify(obj.type == MSGPACK_OBJECT_MAP);
  verify(obj.via.map.size > 0);
  msgpack_object_kv *field = obj.via.map.ptr;
  verify(field[0].key.type == MSGPACK_OBJECT_STR);
  verify(field[0].val.type == MSGPACK_OBJECT_STR);
  verify(msgpack_str_equals(field[0].key.via.str, "hdf5_object"));
  verify(msgpack_str_equals(field[0].val.via.str, "dataset"));

  /* Initialize the output object */
  result.type = HS_DATASET;
  result.dataset.shape = NULL;
  result.dataset.type = NULL;
  result.dataset.attrs.name = NULL;
  result.dataset.attrs.value = NULL;
  result.dataset.data = NULL;

  /* Check for empty place-holder */
  if(obj.via.map.size == 1)return result;

  /* Otherwise, we should also have shape, type, attributes and maybe data fields */
  verify(obj.via.map.size >= 4);
  verify(obj.via.map.size <= 5);
  for(size_t i=0; i<obj.via.map.size; i+=1) {
    verify(field[i].key.type == MSGPACK_OBJECT_STR);
    if(msgpack_str_equals(field[i].key.via.str, "shape")) {
      /* Decode the dataset shape */
      result.dataset.shape = decode_shape(field[i].val);
    } else if(msgpack_str_equals(field[i].key.via.str, "type")) {
      /* Decode the dataset type string */
      result.dataset.type = copy_string(field[i].val);
    } else if(msgpack_str_equals(field[i].key.via.str, "data")) {
      /* Decode the dataset contents */
      result.dataset.data = malloc(sizeof(hs_object));
      *(result.dataset.data) = hs_decode_object(field[i].val);
    } else if(msgpack_str_equals(field[i].key.via.str, "attributes")) {
      /* Decode the dataset's attributes */
      result.group.attrs = decode_attributes(field[i].val);
    } else if(msgpack_str_equals(field[i].key.via.str, "hdf5_object")) {
      /* Nothing to do */
    } else {
      /* Unrecognized field */
      hs_free_object(result);
      result.type = HS_ERROR;
      return result;
    }
  }
  return result;  
}

static hs_object decode_ndarray(msgpack_object obj) {
  verify(obj.type == MSGPACK_OBJECT_MAP);
  hs_object result;
  result.type = HS_ERROR;
  return result;  
}

static hs_object decode_vlen(msgpack_object obj) {
  verify(obj.type == MSGPACK_OBJECT_MAP);
  hs_object result;
  result.type = HS_ERROR;
  return result;  
}

hs_object hs_decode_object(msgpack_object obj) {
  
  /* Check for null object */
  if(obj.type == MSGPACK_OBJECT_NIL) {
    hs_object result;
    result.type = HS_NULL;
    return result;
  }

  /* If not nil, the object should be a msgpack map */
  verify(obj.type == MSGPACK_OBJECT_MAP);

  /* The first entry tells us the object type */
  verify(obj.via.map.size > 0);
  msgpack_object_kv *field = obj.via.map.ptr;
  verify(field[0].key.type == MSGPACK_OBJECT_STR);
  if(msgpack_str_equals(field[0].key.via.str, "hdf5_object")) {
    verify(field[1].key.type == MSGPACK_OBJECT_STR);
    if(msgpack_str_equals(field[0].val.via.str, "group")) {
      /* This is a group */
      return decode_group(obj);
    } else if(msgpack_str_equals(field[0].val.via.str, "dataset")) {
      /* This is a dataset */
      return decode_dataset(obj);
    } else{
      /* Not recognized, so return error */
      hs_object result;
      result.type = HS_ERROR;
      return result;
    }
  } else if(msgpack_str_equals(field[0].key.via.str, "nd")) {
    /* This is an ndarray */
    verify(field[0].val.type == MSGPACK_OBJECT_BOOLEAN);
    return decode_ndarray(obj);
  } else if(msgpack_str_equals(field[0].key.via.str, "vlen")) {
    /* This is a vlen array */
    verify(field[0].val.type == MSGPACK_OBJECT_BOOLEAN);
    return decode_vlen(obj);
  } else {
    /* Not recognized, so return error */
    hs_object result;
    result.type = HS_ERROR;
    return result;
  }
}

int hs_free_object(hs_object obj) {

  /* If the object is null, there's nothing to do*/
  if(obj.type==HS_NULL)return 0;
  
  return -1;
}
