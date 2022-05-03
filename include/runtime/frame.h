#pragma once

#include "common.h"

#include "runtime/stack.h"
#include "runtime/obj.h"
#include "runtime/block.h"

typedef struct fz_frame Frame;

// In order to execute a block the data structures
// required are as follows :-
// 1. A scope for storing the variables in the
// 		current scope.
// All these structures forms a Frame.
// A new frame is created everything an explicit
// block is executed.
struct fz_frame {
	// The stack base pointer.
	Value *base_ptr;

	// The scope object to read from.
	Object *read_scope;

	// The scope object to write to.
	Object *write_scope;

	// The block for which the frame was created.
	Block block;

	// The instruction pointer.
	const uint8_t *ip;
};

extern void frame_init(Frame*);
extern void frame_destroy(Frame*);
