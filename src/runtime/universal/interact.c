#include "runtime/universal/interact.h"
#include "runtime/universal/method.h"
#include "runtime/class.h"
#include "runtime/obj.h"
#include <dlfcn.h>
#include <stdio.h>


Object DYLIB_CLASS;

Value DYLIB_CLASS_NAME = VALUE_CSTRING("DyLibrary", 9);

void dylib_class_init(Object *o) {
	object_init(o);
	class_create(o, NULL, DYLIB_CLASS_NAME);
	o->meta_size = sizeof(DyLibrary);

	ADD_METHOD(o, "loadMethod", dylib_class_method_load_method);
}


char* string_to_cstring(string s) {
	char *p = calloc(s.length+1, sizeof(char));
	memcpy(p, s.value, s.length * sizeof(char));
	return p;
}


Value load_dylib(Object *f, Value me, Value *args, size_t n_args) {
	if (!n_args) {
		return VALUE_NULL;
	}

	char *path;

	Value pv = args[0];
	if (pv.type == VALUE_TYPE_OBJECT && !pv.o_ptr) {
		path = NULL;
	} else if (pv.type != VALUE_TYPE_STRING) {
		return VALUE_NULL;
	} else {
		path = string_to_cstring(pv.s_value);
	}

	void *l = dlopen(path, RTLD_LAZY);

	free(path);

	if (!l) {
		printf("%s\n", dlerror());
		return VALUE_NULL;
	}

	Object *o = class_instantiate(&DYLIB_CLASS);

	DyLibrary *m = o->meta;
	m->lib = l;

	return value_object(o);
}

DEFINE_NATIVE_METHOD(dylib_class_method_load_method) {
	if (me.type != VALUE_TYPE_OBJECT || !me.o_ptr)
		return VALUE_NULL;

	if (!n_args || args[0].type != VALUE_TYPE_STRING) {
		return VALUE_NULL;
	}

	char *n = string_to_cstring(args[0].s_value);

	DyLibrary *l = me.o_ptr->meta;
	NativeFunc *m = dlsym(l->lib, n);

	if (!m) {
		return VALUE_NULL;
	}

	free(n);

	Object *fn;
	fn = class_instantiate(&METHOD_CLASS); 
	Block b; 
	b.start_ptr = (const uint8_t*)m; 
	b.end_ptr = NULL; 
	b.source = NULL; 
	func_create(fn, NULL, b, NULL); 
	func_set_native(fn, TRUE);


	return value_object(fn);
}