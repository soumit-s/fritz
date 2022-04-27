#include "runtime/frame.h"

void frame_init(Frame *f) {
	// Create a new scope.
	f->scope = calloc(1, sizeof(Object));
	object_init(f->scope);
	
	f->base_ptr = NULL;
	f->ip = NULL;
}

void frame_destroy(Frame *f) {
	if (f->scope != NULL) {
		free(f->scope);
	} 
}