#include "runtime/func.h"

void func_create(Object *o, Object *p, Block b, Object *s) {
	// An object of type Function consists
	// of two main properties.
	// 1. params: It is another object containing
	//    info about the parameters.
	// 2. func: Value of type Block
	// 3. scope: The scope in which the function was created
	//  
	// Other properties include, 
	// 3. native?: If the function is a native("C") function
	//    then this value is true, and false otherwise.

	ObjectProperty params, info, native, scope;
	object_property_init(&params);
	object_property_init(&info);
	object_property_init(&native);
	object_property_init(&scope);

	params.key.type = VALUE_TYPE_STRING;
	params.key.s_value = to_string("params");
	params.value.type = VALUE_TYPE_OBJECT;
	params.value.o_ptr = p;

	info.key.type = VALUE_TYPE_STRING;
	info.key.s_value = to_string("func");
	info.value.type = VALUE_TYPE_BLOCK;
	info.value.bl_value = b;

	native.key.type = VALUE_TYPE_STRING;
	native.key.s_value = to_string("native?");
	native.value.type = VALUE_TYPE_BOOLEAN;
	native.value.b_value = FALSE;

	scope.key = value_string(to_string("scope"));
	scope.value = value_object(s);

	ObjectProperty props[4] = {params, info, native, scope};

	object_add_properties(o, props, 4);
}

void func_set_block(Object *o, Block b) {
	object_set_property_value(o, value_string(to_string("func")),
		value_block(b));
}

void func_set_native(Object *o, FzBool b) {
	object_set_property_value(o, value_string(to_string("native?")),
		value_bool(b));
}

Block func_get_block(Object *o) {
	Block b;
	b.end_ptr = NULL;
	b.start_ptr = NULL;
	b.source = NULL;

	ObjectProperty *p = object_get_property(o, value_string(to_string("func")));
	if (p != NULL && p->value.type == VALUE_TYPE_BLOCK) {
		b = p->value.bl_value;
	} 

	return b;
}

FzBool func_get_native(Object *o) {
	ObjectProperty *p = object_get_property(o, value_string(to_string("native?")));
	if (p == NULL || p->value.type != VALUE_TYPE_BOOLEAN) {
		return FALSE;
	} 
	return p->value.b_value == TRUE;
}