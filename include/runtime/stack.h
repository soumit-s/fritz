#pragma once

#include "common.h"
#include "obj.h"

// The stack follows the same conventions
// as that of the stack used by the processor
// with the base pointer pointing to a higher memory address
// and the stack pointer pointing to a lower memory address.
// The stack grows downward in memory. Every time a new value
// is inserted, the stack pointer is decremented by sizeof(Value).
// and the stack pointer is incremented when a value is popped from
// the stack.
typedef struct fz_stack Stack;

struct fz_stack {
	// Stack Capacity.
	size_t capacity;

	Value *values;

	// Top pointer points to the top of the stack.
	Value *top_ptr;
};

extern void stack_init(Stack*);
extern void stack_create(Stack*, size_t);
extern void stack_destroy(Stack*);

// Pushes a Value onto the stack and decrements
// the stack pointer. It takes a Stack object
// and a Value object. 
// NOTE: It does not take pointers.
#define STACK_PUSH(s, v) (*(s).top_ptr++ = (v)) 

// Pops a Value from the stack and increments
// the stack pointer. It takes a Stack object
// and returns a Value object.
#define STACK_POP(s) (*--(s).top_ptr)

#define STACK_GET_FROM_TOP(s, n) (*(s.top_ptr - (n)))
#define STACK_GET_FROM_TOP_PTR(s, n) (s.top_ptr - (n))

#define STACK_SET_TOP(s, t) (s.top_ptr = (t))

