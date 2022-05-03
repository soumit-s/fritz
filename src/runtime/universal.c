#include "runtime/universal.h"
#include "runtime/func.h"

Object UNIVERSAL_SCOPE;

#define ADD_FUNC(scope, name, func) { \
	Object *f = calloc(1, sizeof(Object)); \
	Block b; \
	b.start_ptr = (const uint8_t*)func; \
	b.end_ptr = NULL; \
	b.source = NULL; \
	func_create(f, NULL, b, NULL); \
	func_set_native(f, TRUE); \
	ObjectProperty p; \
	p.key = value_string(to_string(name)); \
	p.value = value_object(f); \
	object_add_property(scope, p); \
}

Value out(Value *values, size_t n_values) {
	for (size_t i=0; i < n_values; ++i) {
		value_log(values[i]);
	}

	return value_object(NULL);
}

// The 'use' method is used to import other fritz
// modules. It takes the path to the modules an an 
// argument and returns an object that contains
// the variables and functions defined by the 
// module.
Value use(Value *values, size_t n_values) {
	return values[0];
}

void universal_scope_create(Object *scope) {
	ADD_FUNC(scope, "out", &out);
	ADD_FUNC(scope, "use", &use);
}