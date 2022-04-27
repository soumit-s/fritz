#include "runtime/fstack.h"

void frame_stack_init(FrameStack *s) {
	s->capacity = 0;
	s->frames = NULL;
	s->top_ptr = NULL;
}

void frame_stack_create(FrameStack *s, size_t n) {
	s->frames = calloc(n, sizeof(Frame));
	s->top_ptr = s->frames;
	s->capacity = n;
}

void frame_stack_destroy(FrameStack *s) {
	if (s->frames == NULL) {
		free(s->frames);
	}
}