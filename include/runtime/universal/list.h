#pragma once

#include "common.h"
#include "runtime/value.h"
#include "runtime/obj.h"

/**
 * Definition for the underlying struct
 * of the List class. 
 * The type of data structure that struct
 * uses is an array list. It has a predefined 
 * capacity which increases. When there is no
 * more room for new elements, then a new array
 * is allocated and the elements are occupied over 
 * to it. The capacity of the new array is 2 times
 * that of the previous capacity.
 **/

typedef struct {

	Value *values;

	// Number of elements
	size_t n_values;

	// List capacity
	size_t c_values;

} List;

extern Value list_class_method_new(Object*, Value me, Value *args, size_t n_args);

extern Value list_class_method_append(Object*, Value, Value*, size_t);

extern Value list_class_method_at(Object*, Value, Value*, size_t);

extern Value list_class_method_length(Object*, Value, Value*, size_t);

extern Value list_class_method___slicer(Object*, Value, Value*, size_t);

extern Object LIST_CLASS;
extern Value LIST_CLASS_NAME;

extern void list_class_init(Object*);