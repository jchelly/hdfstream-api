#ifndef DECODER_H
#define DECODER_H

#include <msgpack.h>

typedef enum hs_type_t {
  HS_ERROR,
  HS_NULL,
  HS_GROUP,
  HS_DATASET,
  HS_NDARRAY,
  HS_VLEN,
  HS_STRING
} hs_type;

typedef struct hs_group_t hs_group;
typedef struct hs_dataset_t hs_dataset;
typedef struct hs_ndarray_t hs_ndarray;
typedef struct hs_vlen_t hs_vlen;
typedef struct hs_object_t hs_object;

typedef struct hs_attrs_t {
  int nr_attrs;
  char **name;
  hs_object *value;
} hs_attrs;

typedef struct hs_group_t {
  int nr_members;
  char **member_name;
  hs_object *member_object;
  hs_attrs attrs;
} hs_group;

typedef struct hs_dataset_t {
  char *type;
  char *kind;
  int rank;
  size_t *shape;
  hs_attrs attrs;
  hs_object *data;
} hs_dataset;

typedef struct hs_ndarray_t {
  char *type;
  char *kind;
  int rank;
  size_t *shape;
  size_t nbytes;
  void *data;
} hs_ndarray;

typedef struct hs_vlen_t {
  int rank;
  size_t *shape;
  size_t nr_elements;
  hs_object *data;
} hs_vlen;

typedef struct hs_object_t {
  hs_type type;
  union {
    hs_group group;
    hs_dataset dataset;
    hs_ndarray ndarray;
    hs_vlen vlen;
    char *string;
  };
} hs_object;

hs_object hs_get_member(hs_object obj, char *name);
hs_object hs_decode_object(msgpack_object obj);
void hs_free_object(hs_object *obj);

#endif
