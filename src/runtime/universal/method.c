#include "runtime/universal/method.h"
#include "runtime/obj.h"
#include "runtime/class.h"
#include "runtime/universal/helper.h"
#include "runtime/func.h"

Object METHOD_CLASS;
Value METHOD_CLASS_NAME = VALUE_CSTRING("Method", 6);

void method_class_init(Object *o) {
	object_init(o);
	class_create(o, NULL, METHOD_CLASS_NAME);
	ADD_METHOD(o, "new", &method_class_new);
}

Value method_class_new(Object *f, Value me, Value *args, Value n_args) {
	return me;
}