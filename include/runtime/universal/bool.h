#pragma once

#include "runtime/universal/helper.h"

extern Object BOOL_CLASS;
extern Value BOOL_CLASS_NAME;

extern void bool_class_init(Object*);

// Bool.__toStr
DECLARE_NATIVE_METHOD(bool_class_method___toStr);