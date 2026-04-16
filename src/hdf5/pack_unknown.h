#ifndef PACK_UNKNOWN_H
#define PACK_UNKNOWN_H

#include <msgpack.h>

/*
  Pack a placeholder for an unknown object type
*/
int pack_unknown(msgpack_packer pk);

#endif
