#include "runtime/obj.h"
#include "runtime/universal/list.h"
#include "runtime/class.h"
#include "runtime/universal/helper.h"
#include "runtime/func.h"
#include "runtime/universal/method.h"

Object LIST_CLASS;
Value LIST_CLASS_NAME = VALUE_CSTRING("List", 4);

void list_class_init(Object *o) {
  object_init(o);
  class_create(o, NULL, LIST_CLASS_NAME);
  o->meta_size = sizeof(List);
  ADD_METHOD(o, "new", &list_class_method_new);
  ADD_METHOD(o, "append", &list_class_method_append);
  ADD_METHOD(o, "at", &list_class_method_at);
  ADD_METHOD(o, "length", &list_class_method_length);
  ADD_METHOD(o, "__slicer", &list_class_method___slicer);
}


Value list_class_method_new(Object *f, Value me, Value *args, size_t n_args) {
	if (me.type != VALUE_TYPE_OBJECT || me.o_ptr == NULL) {
		return VALUE_NULL;
	}
	
	List *l = me.o_ptr->meta;

	l->c_values = n_args + 2;
	l->n_values = n_args;
	l->values = malloc((n_args+2) * sizeof(Value));

	if (n_args) {
		memcpy(l->values, args, n_args * sizeof(Value));
	}

	return me;
}

Value list_class_method_append(Object *f, Value me, Value *args, size_t n_args) {
	if (me.type != VALUE_TYPE_OBJECT || me.o_ptr == NULL
		|| me.o_ptr->meta == NULL || !n_args) {
		return VALUE_NULL;
	}

	List *l = me.o_ptr->meta;

	if (l->n_values >= l->c_values) {
		size_t c = l->c_values * 2;
		Value *v  = malloc(c * sizeof(Value));

		memcpy(v, l->values, l->c_values * sizeof(Value));

		free(l->values);

		l->c_values = c;
		l->values = v;
	}

	size_t i = l->n_values;
	l->values[l->n_values++] = args[0];

	return value_int((FzInt)i);
}

Value list_class_method_at(Object *f, Value me, Value *args, size_t n_args) {
	if (me.type != VALUE_TYPE_OBJECT || me.o_ptr == NULL
		|| me.o_ptr->meta == NULL || !n_args) {
		return VALUE_NULL;
	}

	FzInt i = -1;
	if (n_args && args[0].type == VALUE_TYPE_INT) {
		i = args[0].i_value;
	}

	List *l = me.o_ptr->meta;
	FzInt n = l->n_values;

	i = i < 0 ? n + i: i;

	return i < n ? l->values[i] : VALUE_NULL;
}


Value list_class_method_length(Object *f, Value me, Value *args, size_t n_args) {
	if (me.type != VALUE_TYPE_OBJECT || me.o_ptr == NULL
		|| me.o_ptr->meta == NULL) {
		return VALUE_NULL;
	}

	// This causes clangd to crash
	// (List*)(me.o_ptr->meta)->
	List *l = me.o_ptr->meta;
	return value_int(l->n_values);
}

Value list_class_method___slicer(Object *f, Value me, Value *args, size_t n_args) {
	List *l = me.o_ptr->meta;
	if (n_args == 1 && args[0].type == VALUE_TYPE_INT) {
		FzInt index = args[0].i_value;
		if (index < l->n_values) {
			return l->values[index];
		}
	}

	return VALUE_NULL;
}