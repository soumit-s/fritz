#pragma once

#include "runtime/const_pool.h"
#include "runtime/bcode.h"

typedef enum {
  // External blocks are used while loading modules.
  // Once executed, the write scope is pushed onto
  // the stack. This is internally used by the module.load
  // instruction.
  BLOCK_TYPE_EXTERNAL,

  BLOCK_TYPE_EXPLICIT,
  BLOCK_TYPE_IMPLICIT,

  // Used to start blocks that are not meant
  // to be executed immediately once encountered.
  // It is used in creating methods.
  BLOCK_TYPE_NOEXEC,

  // Used for dependency blocks. These blocks
  // donot have a scope of their own but instead
  // use the scope the parent block.
  BLOCK_TYPE_NOEXEC_IMPLICT
} BLOCK_TYPE;

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

  char *buffer;
  size_t buffer_size;
};

typedef struct fz_block Block;


// A block represents a segment of a
// source that can be executed and has a local scope.
struct fz_block {
  BLOCK_TYPE type;

  const uint8_t *start_ptr;
  const uint8_t *end_ptr;

  // The source, this block belongs to.
  Source *source;

  // Meant for noexec.implicit blocks
  void *read_scope, *write_scope;
};