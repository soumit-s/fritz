#include "runtime/scope.h"
#include "runtime/obj.h"
#include "runtime/universal.h"

const char *SCOPE_PROP_NAME = "scope";
const char *GLOBAL_PROP_NAME = "global";

Object* scope_create(Object *p, Object *g) {
	Object *o = calloc(1, sizeof(Object));
	object_init(o);

	// A scope must contain a property named scope
	// which is a reference to itself.
	ObjectProperty scope;
	object_property_init(&scope);
	
	scope.key.type = VALUE_TYPE_STRING;
	scope.key.s_value = to_string(SCOPE_PROP_NAME);

	scope.value.type = VALUE_TYPE_OBJECT;
	scope.value.o_ptr = o;

	object_add_property(o, scope);

	// A scope must contain a reference to a 
	// global scope. If it does not have a global
	// scope then the property will be NULL.
	ObjectProperty global;
	object_property_init(&global);

	global.key.type = VALUE_TYPE_STRING;
	global.key.s_value = to_string(GLOBAL_PROP_NAME);

	global.value.type = VALUE_TYPE_OBJECT;
	global.value.o_ptr = g;

	object_add_property(o, global);

	// A scope must contain a reference to a 
	// parent scope if it has any.

	return o;
}

ObjectProperty* scope_get_property(Object* o, Value k) {
	ObjectProperty *p;
	if ((p = object_get_property(o, k)) != NULL) {
		return p;
	}

	// If not found in the current scope then 
	// search inside its parent scope. If not
	// found again then got the parent of its 
	// parent scope. This process continues
	// until the top of the scope tree has 
	// been reached.

	Value parent_prop_name;
	parent_prop_name.type = VALUE_TYPE_STRING;
	parent_prop_name.s_value = to_string("parent");

	Object *co = o;

	while(TRUE) {
		ObjectProperty *parent_prop = object_get_property(co, parent_prop_name);
		if (parent_prop == NULL || parent_prop->value.type != VALUE_TYPE_OBJECT) {
			break;
		}

		co = parent_prop->value.o_ptr;
		if (co == NULL) {
			break;
		}

		if ((p = object_get_property(co, k)) != NULL ) {
			return p;
		}
	}

	Value global_prop_name;
	global_prop_name.type = VALUE_TYPE_STRING;
	global_prop_name.s_value = to_string(GLOBAL_PROP_NAME);

	// If not found in parent element then search
	// if the scope has a global scope and search for the property
	// there.
	ObjectProperty *global_prop = object_get_property(o, global_prop_name);
	if (global_prop != NULL) {
		Value v = global_prop->value;
		if (v.type != VALUE_TYPE_OBJECT && v.o_ptr != NULL) {
			p = object_get_property(v.o_ptr, k);
			if (p != NULL) {
				return p;
			}
		}
	}

	// Search in the universal scope.
	p = object_get_property(&UNIVERSAL_SCOPE, k);
	return p;
}