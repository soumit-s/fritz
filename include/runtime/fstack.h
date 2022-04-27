#pragma once
#include "common.h"
#include "runtime/frame.h"

typedef struct fz_frame_stack FrameStack;

struct fz_frame_stack {
	size_t capacity;

	Frame *top_ptr;

	Frame *frames;
};

extern void frame_stack_init(FrameStack*);
extern void frame_stack_create(FrameStack*, size_t);
extern void frame_stack_destroy(FrameStack*);

#define FRAME_STACK_PUSH(s, f) (*s.top_ptr++ = f)
#define FRAME_STACK_POP(s) (*--s.top_ptr)

#define FRAME_STACK_GET_SCOPE(s) ((s.top_ptr-1)->scope)
#define FRAME_STACK_GET(s) (*(s.top_ptr-1))

#define FRAME_STACK_GET_PTR(s) (s.top_ptr-1)

