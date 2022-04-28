#include "runtime/func.h"

void func_create(Object *o) {
	// An object of type Function consists
	// of two main properties.
	// 1. params: It is another object containing
	//    info about the parameters.
	// 2. func: Value of type Block
	//  
	// Other properties include, 
	// 3. native?: If the function is a native("C") function
	//    then this value is true, and false otherwise.

	ObjectProperty params, info, native;

	params.key.type = VALUE_TYPE_STRING;
	params.key.s_value = to_string("params");
	params.value.type = VALUE_TYPE_OBJECT;
	params.value.o_ptr = NULL;

	info.key.type = VALUE_TYPE_STRING;
	info.key.s_value = to_string("func");
	info.value.type = VALUE_TYPE_BLOCK;

	native.key.type = VALUE_TYPE_STRING;
	native.key.s_value = to_string("native?");
	native.value.type = VALUE_TYPE_BOOLEAN;
	native.value.b_value = FALSE;

	ObjectProperty props[3] = {params, info, native};

	object_add_properties(o, props, 3);
}

void func_set_block(Object *o, Block b) {
	object_set_property(o, value_string(to_string("func")),
		value_block(b));
}

void func_set_native(Object *o, FzBool b) {
	object_set_property(o, value_string(to_string("native?")),
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