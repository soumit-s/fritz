#pragma once

#include "runtime/universal/helper.h"

extern Object INT_CLASS;
extern Value INT_CLASS_NAME;

extern void int_class_init(Object*);

// Int.__toStr
DECLARE_NATIVE_METHOD(int_class_method___toStr);
