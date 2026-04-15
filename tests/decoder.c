#include "decoder.h"
#include "verify.h"
#include <msgpack.h>

static size_t msgpack_integer_as_size_t(const msgpack_object obj) {
  verify(obj.type == MSGPACK_OBJECT_POSITIVE_INTEGER);
  return obj.via.u64;
}

/* Compare a msgpack string to a C null terminated string */
static bool msgpack_str_equals(const msgpack_object obj, const char *str) {
  verify(obj.type == MSGPACK_OBJECT_STR);
  size_t n = strlen(str);
  if(n != obj.via.str.size)return false;
  return (memcmp(obj.via.str.ptr, str, n) == 0);
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
  verify(msgpack_str_equals(field[0].key, "hdf5_object"));
  verify(msgpack_str_equals(field[0].val, "group"));

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
  for(size_t i=1; i<obj.via.map.size; i+=1) {
    verify(field[i].key.type == MSGPACK_OBJECT_STR);
    if(msgpack_str_equals(field[i].key, "members")) {
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
    } else if(msgpack_str_equals(field[i].key, "attributes")) {
      /* Decode the group's attributes */
      result.group.attrs = decode_attributes(field[i].val);
    } else {
      /* Unrecognized field */
      hs_free_object(&result);
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
    shape[i] = msgpack_integer_as_size_t(obj.via.array.ptr[i]);
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
  verify(msgpack_str_equals(field[0].key, "hdf5_object"));
  verify(msgpack_str_equals(field[0].val, "dataset"));

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
  for(size_t i=1; i<obj.via.map.size; i+=1) {
    verify(field[i].key.type == MSGPACK_OBJECT_STR);
    if(msgpack_str_equals(field[i].key, "shape")) {
      /* Decode the dataset shape */
      result.dataset.shape = decode_shape(field[i].val);
    } else if(msgpack_str_equals(field[i].key, "type")) {
      /* Decode the dataset type string */
      result.dataset.type = copy_string(field[i].val);
    } else if(msgpack_str_equals(field[i].key, "data")) {
      /* Decode the dataset contents */
      result.dataset.data = malloc(sizeof(hs_object));
      *(result.dataset.data) = hs_decode_object(field[i].val);
    } else if(msgpack_str_equals(field[i].key, "attributes")) {
      /* Decode the dataset's attributes */
      result.group.attrs = decode_attributes(field[i].val);
    } else {
      /* Unrecognized field */
      hs_free_object(&result);
      result.type = HS_ERROR;
      return result;
    }
  }
  return result;
}

static hs_object decode_ndarray(msgpack_object obj) {

  /* Initialize the output object */
  hs_object result;
  result.type = HS_NDARRAY;
  result.ndarray.type = NULL;
  result.ndarray.rank = -1;
  result.ndarray.shape = NULL;
  result.ndarray.nbytes = 0;
  result.ndarray.data = NULL;

  /* Interpret the map fields */
  int have_nbytes = 0;
  verify(obj.type == MSGPACK_OBJECT_MAP);
  msgpack_object_kv *field = obj.via.map.ptr;
  verify(msgpack_str_equals(field[0].key, "nd"));
  verify(obj.via.map.size == 6);
  for(size_t i=1; i<obj.via.map.size; i+=1) {
    verify(field[i].key.type == MSGPACK_OBJECT_STR);
    if(msgpack_str_equals(field[i].key, "type")) {
      result.ndarray.type = copy_string(field[i].val);
    } else if (msgpack_str_equals(field[i].key, "kind")) {
      result.ndarray.kind = copy_string(field[i].val);
    } else if (msgpack_str_equals(field[i].key, "shape")) {
      result.ndarray.shape = decode_shape(field[i].val);
    } else if (msgpack_str_equals(field[i].key, "nbytes")) {
      result.ndarray.nbytes = msgpack_integer_as_size_t(field[i].val);
      have_nbytes = 1;
    } else if (msgpack_str_equals(field[i].key, "data")) {
      verify(have_nbytes); /* Must appear *before* the data */
      verify(field[i].val.type == MSGPACK_OBJECT_ARRAY);
      result.ndarray.data = malloc(result.ndarray.nbytes);
      /* Copy data from the msgpack bin objects */
      msgpack_object_array *array = &(field[i].val.via.array);
      size_t nr_buffers = array->size;
      size_t nr_bytes_copied = 0;
      for(size_t buffer_nr=0; buffer_nr<nr_buffers; buffer_nr+=1) {
	/* All array elements should be bin objects */
	verify(array->ptr[buffer_nr].type == MSGPACK_OBJECT_BIN);
	/* Locate the current bin object */
	msgpack_object_bin *bin = &(array->ptr[buffer_nr].via.bin);
	/* Bounds check number of bytes to copy */
	verify(nr_bytes_copied+bin->size <= result.ndarray.nbytes);
	/* Copy the data */
	memcpy(((char *) result.ndarray.data)+nr_bytes_copied, bin->ptr, bin->size);
	/* Update number of bytes copied */
	nr_bytes_copied += bin->size;
      }
      verify(nr_bytes_copied == result.ndarray.nbytes);
    } else {
      /* Unrecognized field */
      hs_free_object(&result);
      result.type = HS_ERROR;
      return result;
    }
  }
  return result;
}

static hs_object decode_vlen(msgpack_object obj) {

  /* Initialize the output object */
  hs_object result;
  result.type = HS_VLEN;
  result.vlen.rank = -1;
  result.vlen.shape = NULL;
  result.vlen.data = NULL;

  /* Interpret the map fields */
  verify(obj.type == MSGPACK_OBJECT_MAP);
  msgpack_object_kv *field = obj.via.map.ptr;
  verify(msgpack_str_equals(field[0].key, "vlen"));
  verify(obj.via.map.size == 3);
  for(size_t i=1; i<obj.via.map.size; i+=1) {
    verify(field[i].key.type == MSGPACK_OBJECT_STR);
    if(msgpack_str_equals(field[i].key, "shape")) {
      /* Store shape of the array */
      result.vlen.shape = decode_shape(field[i].val);
    } else if(msgpack_str_equals(field[i].key, "data")) {
      /* Field value should be an array */
      verify(field[i].val.type == MSGPACK_OBJECT_ARRAY);
      msgpack_object_array *array = &(field[i].val.via.array);
      /* Determine number of elements */
      result.vlen.nr_elements = array->size;
      /* Check against the shape */
      verify(result.vlen.shape != NULL);
      size_t n = 1;
      for(int dim_nr=0; dim_nr<result.vlen.rank; dim_nr+=1)
	n *= result.vlen.shape[dim_nr];
      verify(n == result.vlen.nr_elements);
      /* Allocate array of element objects */
      result.vlen.data = malloc(n*sizeof(hs_object));
      /* Decode the elements */
      for(size_t element_nr=0; element_nr<n; element_nr+=1)
	result.vlen.data[element_nr] = hs_decode_object(array->ptr[element_nr]);
    } else {
      /* Unrecognized field */
      hs_free_object(&result);
      result.type = HS_ERROR;
      return result;
    }
  }
  return result;
}

hs_object hs_decode_object(msgpack_object obj) {

  /* Check for null object */
  if(obj.type == MSGPACK_OBJECT_NIL) {
    hs_object result;
    result.type = HS_NULL;
    return result;
  }

  /* Check for a string object */
  if(obj.type == MSGPACK_OBJECT_STR) {
    hs_object result;
    result.type = HS_STRING;
    result.string = copy_string(obj);
    return result;
  }

  /* If not nil or string, the object should be a msgpack map */
  verify(obj.type == MSGPACK_OBJECT_MAP);

  /* The first entry tells us the object type */
  verify(obj.via.map.size > 0);
  msgpack_object_kv *field = obj.via.map.ptr;
  verify(field[0].key.type == MSGPACK_OBJECT_STR);
  if(msgpack_str_equals(field[0].key, "hdf5_object")) {
    verify(field[0].val.type == MSGPACK_OBJECT_STR);
    if(msgpack_str_equals(field[0].val, "group")) {
      /* This is a group */
      return decode_group(obj);
    } else if(msgpack_str_equals(field[0].val, "dataset")) {
      /* This is a dataset */
      return decode_dataset(obj);
    } else{
      /* Not recognized, so return error */
      hs_object result;
      result.type = HS_ERROR;
      return result;
    }
  } else if(msgpack_str_equals(field[0].key, "nd")) {
    /* This is an ndarray */
    verify(field[0].val.type == MSGPACK_OBJECT_BOOLEAN);
    return decode_ndarray(obj);
  } else if(msgpack_str_equals(field[0].key, "vlen")) {
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

static void free_array_of_strings(size_t n, char ***arr) {
  if(*arr==NULL)return; /* Do nothing if already null */
  for(size_t i=0; i<n; i+=1)
    free(*arr+i);
  free(*arr);
  *arr = NULL;
}

static void free_array_of_hs_object(size_t n, hs_object **arr) {
  if(*arr==NULL)return; /* Do nothing if already null */
  for(size_t i=0; i<n; i+=1)
    hs_free_object(*arr+i);
  free(*arr);
  *arr = NULL;
}

void hs_free_object(hs_object *obj) {

  /* If the object is null, there's nothing to do*/
  if(obj->type==HS_NULL)return;

  /* If the object is a string, we can just free it */
  if(obj->type==HS_STRING) {
    if(obj->string)free(obj->string);
    obj->string = NULL;
    obj->type = HS_NULL;
    return;
  }

  /* Check if it's a group */
  if(obj->type==HS_GROUP) {
    /* Free members */
    free_array_of_hs_object(obj->group.nr_members, &obj->group.member_object);
    /* Free member names */
    free_array_of_strings(obj->group.nr_members, &obj->group.member_name);
    /* Free attributes */
    free_array_of_strings(obj->group.attrs.nr_attrs, &obj->group.attrs.name);
    free_array_of_hs_object(obj->group.attrs.nr_attrs, &obj->group.attrs.value);
    /* Set this object to null */
    obj->type = HS_NULL;
    return;
  }

  /* Check if it's a dataset */
  if(obj->type==HS_DATASET) {
    if(obj->dataset.type) {
      free(obj->dataset.type);
      obj->dataset.type = NULL;
    }
    if(obj->dataset.shape) {
      free(obj->dataset.shape);
      obj->dataset.shape = NULL;
    }
    if(obj->dataset.data) {
      hs_free_object(obj->dataset.data);
      obj->dataset.data = NULL;
    }
    /* Set this object to null */
    obj->type = HS_NULL;
    return;
  }

  /* Check if it's an ndarray */
  if(obj->type==HS_NDARRAY) {
    if(obj->ndarray.type) {
      free(obj->ndarray.type);
      obj->ndarray.type = NULL;
    }
    if(obj->ndarray.kind) {
      free(obj->ndarray.kind);
      obj->ndarray.kind = NULL;
    }
    if(obj->ndarray.shape) {
      free(obj->ndarray.shape);
      obj->ndarray.shape = NULL;
    }
    if(obj->ndarray.data) {
      free(obj->ndarray.data);
      obj->ndarray.data = NULL;
    }
    /* Set this object to null */
    obj->type = HS_NULL;
    return;
  }

  /* Check if it's a vlen array */
  if(obj->type==HS_VLEN) {
    if(obj->vlen.shape) {
      free(obj->vlen.shape);
      obj->vlen.shape = NULL;
    }
    if(obj->vlen.data) {
      for(size_t element_nr=0; element_nr<obj->vlen.nr_elements; element_nr+=1)
	hs_free_object(obj->vlen.data+element_nr);
      free(obj->vlen.data);
    }
    /* Set this object to null */
    obj->type = HS_NULL;
    return;
  }
}
