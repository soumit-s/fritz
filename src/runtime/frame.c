#include "runtime/frame.h"

void frame_init(Frame *f) {
	// Create a new scope.
	f->scope = NULL;	
	f->base_ptr = NULL;
	f->ip = NULL;
}

void frame_destroy(Frame *f) {
	//if (f->scope != NULL) {
	//	free(f->scope);
	//} 
}