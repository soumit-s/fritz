#include "runtime/universal/object.h"

#include "runtime/class.h"
#include "runtime/obj.h"
#include "runtime/universal/helper.h"
#include "runtime/universal/method.h"
#include "runtime/universal/list.h"

Object OBJECT_CLASS;
Value OBJECT_CLASS_NAME = VALUE_CSTRING("Object", 6);

void object_class_init(Object* o) {
	object_init(o);
	class_create(o, NULL, OBJECT_CLASS_NAME);

	ADD_METHOD(o, "new", &object_class_method_new);
	ADD_METHOD(o, "__keys", &object_class_method_keys);
	ADD_METHOD(o, "__hasKey", &object_class_method_has_key);
}

DEFINE_NATIVE_METHOD(object_class_method_new) {
	return me;
}

DEFINE_NATIVE_METHOD(object_class_method_keys) {
	if (me.type != VALUE_TYPE_OBJECT || !me.o_ptr) {
		return VALUE_NULL;
	}

	Object *l = class_instantiate(&LIST_CLASS);
	List m;
	m.c_values = me.o_ptr->n_props;
	m.n_values = m.c_values;
	m.values = malloc(m.c_values * sizeof(Value));

	for (size_t n = m.c_values; n > 0;) {
		ObjectProperty p = me.o_ptr->props[--n];
		m.values[n] = p.key;
	}

	*((List*)l->meta) = m;

	return value_object(l);
}

DEFINE_NATIVE_METHOD(object_class_method_has_key) {
	return VALUE_FALSE;
}



