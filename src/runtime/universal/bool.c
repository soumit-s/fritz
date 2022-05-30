#include "runtime/universal/bool.h"

#include "runtime/class.h"
#include "runtime/universal/method.h"

Object BOOL_CLASS;
Value BOOL_CLASS_NAME = VALUE_CSTRING("Bool", 4);

void bool_class_init(Object *o) {
	object_init(o);
	class_create(o, NULL, BOOL_CLASS_NAME);

	ADD_METHOD(o, "__toStr", &bool_class_method___toStr);
}

DEFINE_NATIVE_METHOD(bool_class_method___toStr) {
	if (me.type != VALUE_TYPE_BOOLEAN) {
		return VALUE_NULL;
	}

	return me.b_value ? value_cstring("true") : value_cstring("false");
}