#include "runtime/obj.h"
#include "runtime/class.h"
#include "runtime/universal/object.h"
#include <stdio.h>

const Value VALUE_NULL = VALUE_OBJECT(NULL);
const Value VALUE_FALSE = VALUE_BOOLEAN(FALSE);
const Value VALUE_TRUE = VALUE_BOOLEAN(TRUE);

const Value PROP_PAYLOAD_GETTER_KEY = VALUE_CSTRING("get", 3);
const Value PROP_PAYLOAD_SETTER_KEY = VALUE_CSTRING("set", 3);

#define X(a, b, c, d) Value value_##b(c x) { Value v; v.type=VALUE_TYPE_##a; v.d=x; return v; }
	VALUE_TYPE_PREFIXES
#undef X

void object_property_init(ObjectProperty *p) {
	p->setter = NULL;
	p->getter = NULL;
	p->is_mutable = FALSE;
	p->is_behaviour = FALSE;
	p->is_hidden = FALSE;
	p->deps = NULL;
	p->n_deps = 0;
}

void object_property_add_dependant(ObjectProperty *p, Block b) {
	Block *deps = malloc((p->n_deps + 1) * sizeof(Block));
	memcpy(deps, p->deps, p->n_deps * sizeof(Block));
	deps[p->n_deps++] = b;
	free(p->deps);
	p->deps = deps;
}

void object_property_attach_payload(ObjectProperty *p, Object *o) {
	if (!o || !p) {
		return;
	}

	ObjectProperty *t;

	t = object_find_member(o, PROP_PAYLOAD_GETTER_KEY);
	if (t && t->value.type == VALUE_TYPE_OBJECT) {
		p->getter = t->value.o_ptr;
	}

	t = object_find_member(o, PROP_PAYLOAD_SETTER_KEY);
	if (t && t->value.type == VALUE_TYPE_OBJECT) {
		p->setter = t->value.o_ptr;
	}
}

void object_init(Object *o) {
	o->blueprint = NULL;
	o->n_props = 0;
	o->props = NULL;
	o->meta = NULL;
	o->meta_size = 0;
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
	ObjectProperty *p = NULL;
	// Search for the property in the object.
	size_t i = object_search_property(o, k);
	// Search for the property in the blueprint
	// of the current object, if it has one.
	if (i != -1) {
		p = &o->props[i];
	} else if (o->blueprint &&
			(i = object_search_property(o->blueprint, k)) != -1) {
		p = &(o->blueprint->props[i]);
	}

	return p;
}

void object_set_property(Object *o, ObjectProperty prop) {
	size_t i = object_search_property(o, prop.key);

	if (i == -1) {
		object_add_property(o, prop); 
	} else {
		o->props[i] = prop;
	}
}

void object_set_property_value(Object *o, Value k, Value v) {
	size_t i = object_search_property(o, k);

	if (i == -1) {
		ObjectProperty prop;
		object_property_init(&prop);
		prop.key = k;
		prop.value = v;
		object_add_property(o, prop); 
	} else {
		// If value is a string then deallocate it
		if (o->props[i].value.type == VALUE_TYPE_STRING) {
			free((char*)o->props[i].value.s_value.value);
		}

		o->props[i].value = v;;
	}
}

ObjectProperty* object_find_property(Object *o, Value k) {
	ObjectProperty *props = o->props;

	for (size_t i = 0, n=o->n_props; i < n; ++i) {
		if (value_equals(props[i].key, k)) {
			return &props[i];
		}
	}

	return NULL;
}

ObjectProperty* object_find_member(Object *o, Value k) {
	
	ObjectProperty *props = o->props;

	for (size_t i = 0, n=o->n_props; i < n; ++i) {
		if (value_equals(props[i].key, k)) {
			return &props[i];
		}
	}

	// If the object has a blueprint, then
	// search inside it.
	Object *b = o->blueprint;
	while(b) {
		props = b->props;
		for (size_t i=0, n=b->n_props; i < n; ++i) {
			if (value_equals(props[i].key, k)){
				return &props[i];
			}
		}

		ObjectProperty *p = object_get_property(b, SUPER_PROP_NAME);
		if (p && p->value.type == VALUE_TYPE_OBJECT) {
			b = p->value.o_ptr;
		} else {
			break;
		}
	}

	// Search inside the object class.
	for (size_t i=0; i < OBJECT_CLASS.n_props; ++i) {
		if (value_equals(OBJECT_CLASS.props[i].key, k)) {
			return &OBJECT_CLASS.props[i];
		}
	}


	return NULL;
}

int object_set_member_value(Object *o, Value k, Value v) {
	ObjectProperty *m = object_find_member(o, k);
	if (m) {
		m->value = v;
	} else {
		ObjectProperty p;
		object_property_init(&p);
		p.key = k;
		p.value = v;

		object_add_property(o, p);
	}
	return TRUE;
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

void value_log_depth(Value, int);
void object_log_depth(Object*, int);

void value_log_depth(Value v, int depth) {
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
			for (size_t i=0; i < v.s_value.length; ++i) {
				//if (v.s_value.value[i] == '\n') {
				//	printf("\033[;38m\\n\033[0");
				//} else {
					printf("%c", v.s_value.value[i]);
				//}
			}
			break;
		case VALUE_TYPE_OBJECT:
			if (depth > 0) { 
				object_log_depth(v.o_ptr, depth-1);
			} else {
				printf("[Object]");
			}
			break;
		case VALUE_TYPE_BLOCK:
			printf("[Block]");
			break;
	}
}

void value_log(Value v) {
	value_log_depth(v, 3);
}


void object_log_depth(Object *o, int depth) {
	if (o == NULL) {
		printf("null");
		return;
	}
	printf("{ ");
	for (size_t i=0; i < o->n_props; ++i) {
		value_log_depth(o->props[i].key, depth-1);
		printf(": ");
		Value v = o->props[i].value;
		if (v.o_ptr == o)
			printf("[self]");
		else
			value_log_depth(o->props[i].value, depth-1);

		if (i < o->n_props-1)
			printf(", ");
		else
			printf(" ");
	}
	printf("}");
} 

void object_log(Object *o) {
	object_log_depth(o, 3);
}
