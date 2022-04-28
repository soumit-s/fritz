#pragma once

#include "runtime/const_pool.h"
#include "runtime/bcode.h"

// Id of a Source Object.
typedef size_t SOURCE_ID;

typedef struct fz_source Source;

// Source represents a source of bytecode.
// It contains pointer to the Bcode object along
// with the meta data about the origin of the bytecode
// and unique id for the source (ID cannot be 0).
struct fz_source {
  SOURCE_ID id;
  Bcode bcode;
  BcodeMetaData data;
  ConstantPool const_pool;
};

typedef struct fz_block Block;


// A block represents a segment of a
// source that can be executed and has a local scope.
struct fz_block {
  const uint8_t *start_ptr;
  const uint8_t *end_ptr;

  // The source, this block belongs to.
  Source *source;
};