#pragma once

#include "runtime/func.h"

#define DEFINE_NATIVE_METHOD(name) Value name (Object *f, Value me, Value *args, size_t n_args)
#define DECLARE_NATIVE_METHOD(name) extern DEFINE_NATIVE_METHOD(name)


/**
 * Macro for easily defining a class method
 * to a class object.
 * Note: This macro is DEPECRATED. Instead
 * use the ADD_METHOD macro.
 */
#define ADD_FUNC(scope, name, func) { \
	Object *f = class_instantiate(&METHOD_CLASS); \
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


/**
 * Macro for easily defining a class method
 * to a class object. The macro requires
 * a pointer to the class object, the method
 * name and a function pointer for the underlying
 * 'C' function.
 */
#define ADD_METHOD(obj, name, func) ADD_FUNC(obj, name, func) 


