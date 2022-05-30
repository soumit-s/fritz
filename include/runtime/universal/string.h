#pragma once

#include "runtime/universal/helper.h"
#include "runtime/obj.h"

extern Object STRING_CLASS;
extern Value STRING_CLASS_NAME;

extern void string_class_init(Object*);

// Returns number of characters in the
// string.
DECLARE_NATIVE_METHOD(string_class_method_len);

// Returns the size of string in bytes.
DECLARE_NATIVE_METHOD(string_class_method_size);

DECLARE_NATIVE_METHOD(string_class_method___slicer);