#include "runtime/universal.h"
#include "runtime/func.h"
#include "runtime/class.h"
#include "runtime/instance.h"
#include "runtime/obj.h"
#include "runtime/universal/list.h"
#include "runtime/universal/method.h"
#include "runtime/universal/object.h"
#include "runtime/universal/string.h"
#include "runtime/universal/interact.h"
#include "runtime/universal/int.h"
#include "runtime/universal/float.h"
#include "runtime/universal/bool.h"

Object UNIVERSAL_SCOPE;

/*#define ADD_FUNC(scope, name, func) { \
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
}*/


Value out(Object *f, Value me, Value *values, size_t n_values) {
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
Value use(Value me, Value *values, size_t n_values) {
	if (n_values == 0) {
		// error.
	}

	Value path = values[0];

	if (path.type != VALUE_TYPE_STRING) {
		// error: path should be a string
	}

	return values[0];
}


void universal_scope_create(Object *scope) {
	// Method class must be initialized first.
	method_class_init(&METHOD_CLASS);

	string_class_init(&STRING_CLASS);

	int_class_init(&INT_CLASS);

	float_class_init(&FLOAT_CLASS);

	bool_class_init(&BOOL_CLASS);


	ADD_FUNC(scope, "out", &out);
	ADD_FUNC(scope, "loadDylib", &load_dylib);
	//ADD_FUNC(scope, "use", &use);

	dylib_class_init(&DYLIB_CLASS);
	// List class initialization
	list_class_init(&LIST_CLASS);
	// Object class initialization
	object_class_init(&OBJECT_CLASS);

	object_set_property_value(scope, METHOD_CLASS_NAME, value_object(&METHOD_CLASS));	
	object_set_property_value(scope, LIST_CLASS_NAME, value_object(&LIST_CLASS));
	object_set_property_value(scope, OBJECT_CLASS_NAME, value_object(&OBJECT_CLASS));
	object_set_property_value(scope, DYLIB_CLASS_NAME, value_object(&DYLIB_CLASS));
	object_set_property_value(scope, STRING_CLASS_NAME, value_object(&STRING_CLASS));
	object_set_property_value(scope, INT_CLASS_NAME, value_object(&INT_CLASS));
	object_set_property_value(scope, FLOAT_CLASS_NAME, value_object(&FLOAT_CLASS));
	object_set_property_value(scope, BOOL_CLASS_NAME, value_object(&BOOL_CLASS));
}