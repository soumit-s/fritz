#pragma once

#include "runtime/universal/helper.h"

extern Object FLOAT_CLASS;
extern Value FLOAT_CLASS_NAME;

extern void float_class_init(Object*);

// Float.__toStr
DECLARE_NATIVE_METHOD(float_class_method___toStr);