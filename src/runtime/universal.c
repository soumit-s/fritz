#include "runtime/universal.h"
#include "runtime/func.h"

Object UNIVERSAL_SCOPE;

void out(Value *values, size_t n_values) {
	for (size_t i=0; i < n_values; ++i) {
		value_log(values[i]);
	}
}

void universal_scope_create(Object *scope) {

	Object *out_func = calloc(1, sizeof(Object));
	func_create(out_func);

	Block out_block;
	out_block.start_ptr = (const uint8_t*)&out;
	out_block.end_ptr = NULL;
	out_block.source = NULL;

	func_set_block(out_func, out_block);
	func_set_native(out_func, TRUE);

	ObjectProperty out_func_prop;
	out_func_prop.key = value_string(to_string("out"));
	out_func_prop.value = value_object(out_func);

	object_add_property(scope, out_func_prop);
}