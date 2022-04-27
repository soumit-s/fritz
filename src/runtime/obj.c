#include "runtime/obj.h"


void object_property_init(ObjectProperty *p) {
	p->is_mutable = FALSE;
	p->is_behaviour = FALSE;
}

void object_init(Object *o) {
	o->n_props = 0;
	o->props = NULL;
}

void object_add_property(Object *o, ObjectProperty p) {
	ObjectProperty *props = calloc(o->n_props + 1, sizeof(ObjectProperty));
	if (o->props != NULL) { 
		memcpy(props, o->props, o->n_props);
		free(o->props);
	}
	o->props = props;

	props[o->n_props++] = p;
}

int object_search_nth_property(Object *o, Value v, int n) {
	for (size_t i=0; i < o->n_props; ++i) {
		ObjectProperty p = o->props[i];
		if (compare_values(p.value, v)) {
			n--;
			if (!n) {
				return i;
			}
		}
	}
	return -1;
}

ObjectProperty* object_get_property(Object *o, Value k) {
	size_t i = object_search_property(o, k);
	return i == -1 ? NULL : &o->props[i]; 
}


int compare_values(Value v1, Value v2) {
	if (v1.type == v2.type) {
		switch (v1.type) {
			case VALUE_TYPE_BOOLEAN:
				return v1.b_value | v2.b_value;
			case VALUE_TYPE_INT:
				return v1.i_value == v2.i_value;
			case VALUE_TYPE_FLOAT:
				return v1.f_value == v2.f_value;
			case VALUE_TYPE_STRING:
				return string_eq(v1.s_value, v2.s_value);
			case VALUE_TYPE_OBJECT:
				return v1.o_ptr == v2.o_ptr;
		}
	} else if (v1.type == VALUE_TYPE_INT && v2.type == VALUE_TYPE_FLOAT) {
		return v1.i_value == v2.f_value;
	} else if (v1.type == VALUE_TYPE_FLOAT && v2.type == VALUE_TYPE_INT) {
		return v1.f_value == v2.i_value;
	}

	return FALSE;
}
