#pragma once

#include "runtime/obj.h"

/**
 * Definition for the native Method class which
 * serves as the base class for all callable
 * objects and the default type for methods.
 **/

typedef struct {

	// Used to store the 'me' object of
	// the function.
	Object *me;
} Method;

extern void method_class_init(Object*);

extern Value method_class_new(Object*, Value, Value *args, Value n_args);

extern Object METHOD_CLASS;
extern Value METHOD_CLASS_NAME;