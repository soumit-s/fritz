#include "runtime/obj.h"
#include <stdio.h>

#define X(a, b, c, d) Value value_##b(c x) { Value v; v.type=VALUE_TYPE_##a; v.d=x; return v; }
	VALUE_TYPE_PREFIXES
#undef X

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
		memcpy(props, o->props, o->n_props * sizeof(ObjectProperty));
		free(o->props);
	}
	o->props = props;

	props[o->n_props++] = p;
}

void object_add_properties(Object *o, ObjectProperty *p, size_t n_props) {
	ObjectProperty *props = calloc(o->n_props + n_props, sizeof(ObjectProperty));
	if (o->props != NULL) { 
		memcpy(props, o->props, o->n_props * sizeof(ObjectProperty));
		free(o->props);
	}
	o->props = props;

	memcpy(o->props + o->n_props, p, n_props * sizeof(ObjectProperty));
	o->n_props += n_props;
}

int object_search_nth_property(Object *o, Value v, int n) {
	for (size_t i=0; i < o->n_props; ++i) {
		ObjectProperty p = o->props[i];
		if (value_equals(p.key, v)) {
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

void object_set_property(Object *o, Value k, Value v) {
	ObjectProperty *p = object_get_property(o, k);
	if (p == NULL) {
		ObjectProperty x;
		x.key = k;
		x.value = v;
		object_add_property(o, x); 
	} else {
		p->value = v;
	}
}

int value_equals(Value v1, Value v2) {
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
			case VALUE_TYPE_BLOCK:
				return FALSE;
		}
	} else if (v1.type == VALUE_TYPE_INT && v2.type == VALUE_TYPE_FLOAT) {
		return v1.i_value == v2.f_value;
	} else if (v1.type == VALUE_TYPE_FLOAT && v2.type == VALUE_TYPE_INT) {
		return v1.f_value == v2.i_value;
	}

	return FALSE;
}

void value_log(Value v) {
	switch (v.type) {
		case VALUE_TYPE_FLOAT:
			printf("%lf", v.f_value);
			break;
		case VALUE_TYPE_INT:
			printf("%ld", v.i_value);
			break;
		case VALUE_TYPE_BOOLEAN:
			printf("%s", v.b_value ? "true" : "false");
			break;
		case VALUE_TYPE_STRING:
			for (size_t i=0; i < v.s_value.length; ++i)
				printf("%c", v.s_value.value[i]);
			break;
		case VALUE_TYPE_OBJECT:
			printf("[Object]");
			break;
		case VALUE_TYPE_BLOCK:
			printf("[Block]");
			break;
	}
}


void object_log(Object *o) {
	printf("{ ");
	for (size_t i=0; i < o->n_props; ++i) {
		value_log(o->props[i].key);
		printf(": ");
		value_log(o->props[i].value);
		printf(", ");
	}
	printf("}");
} 