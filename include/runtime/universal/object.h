#pragma once

#include "runtime/obj.h"

extern Object OBJECT_CLASS;
extern Value OBJECT_CLASS_NAME;

/**
 * The Object class is the implicit base class
 * for all objects. It contains utilities for
 * accessing and manipulating the object properties
 * The methods that are part of the object class 
 * are as follows :-
 * 
 * 1. __keys()
 * 
 * 2. __set(n, v)
 * 
 * 3. __hasKey(n)
 * 
 */

extern void object_class_init(Object*);

extern Value object_class_method_new(Object*, Value, Value*, size_t);

/**
 * Underlying function for the method __keys()
 * The function takes no parameters.
 * It returns a List object containing 
 * the property names of the object.
 */
extern Value object_class_method_keys(Object*, Value, Value*, size_t);

/**
 * Underlying function for the method __hasKey(n)
 * The function takes the name of the property to
 * search for in the object and returns true if
 * the property is found, false otherwise.
 */
extern Value object_class_method_has_key(Object*, Value, Value*, size_t);