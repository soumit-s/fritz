#pragma once

#include "runtime/obj.h"

extern const Value FUNC_ME_PROP_NAME;

typedef Value (*NativeFunc)(Object*, Value, Value*, size_t);

extern void func_create(Object *o, Object *params, Block b, Object *scope);

extern void func_set_block(Object*, Block);

extern void func_set_native(Object*, FzBool);

extern FzBool func_get_native(Object*);

extern Block func_get_block(Object*);