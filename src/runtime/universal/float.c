#include "runtime/universal/float.h"

#include "runtime/class.h"
#include "runtime/universal/method.h"

Object FLOAT_CLASS;
Value FLOAT_CLASS_NAME = VALUE_CSTRING("Float", 5);

void float_class_init(Object *o) {
	object_init(o);
	class_create(o, NULL, FLOAT_CLASS_NAME);

	ADD_METHOD(o, "__toStr", &float_class_method___toStr);
}

DEFINE_NATIVE_METHOD(float_class_method___toStr) {
	return VALUE_NULL;
}

