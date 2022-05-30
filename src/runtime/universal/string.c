#include "runtime/universal/string.h"

#include "runtime/class.h"
#include "runtime/universal/helper.h"
#include "runtime/universal/method.h"

Object STRING_CLASS;
Value STRING_CLASS_NAME = VALUE_CSTRING("String", 6);

void string_class_init(Object *o) {
	object_init(&STRING_CLASS);
	class_create(&STRING_CLASS, NULL, STRING_CLASS_NAME);
	ADD_METHOD(&STRING_CLASS, "size", &string_class_method_size);
	ADD_METHOD(&STRING_CLASS, "len", &string_class_method_len);
	ADD_METHOD(&STRING_CLASS, "__slicer", &string_class_method___slicer);
}

DEFINE_NATIVE_METHOD(string_class_method_len) {
	return VALUE_NULL;
}

DEFINE_NATIVE_METHOD(string_class_method_size) {
	return value_int(me.s_value.length);
}

DEFINE_NATIVE_METHOD(string_class_method___slicer) {
	return VALUE_NULL;
}