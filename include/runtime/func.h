#pragma once

#include "runtime/obj.h"

typedef void (*NativeFunc)(Value*, size_t);

extern void func_create(Object *o);

extern void func_set_block(Object*, Block);

extern void func_set_native(Object*, FzBool);

extern FzBool func_get_native(Object*);

extern Block func_get_block(Object*);