#pragma once

#include "runtime/obj.h"

#include "runtime/universal/helper.h"

#ifdef WIN32
#	define __PLATFORM_WINDOWS
#else
#	define __PLATFORM_LINUX
#endif

#ifdef __PLATFORM_LINUX
	#include <dlfcn.h>
	#include <sys/types.h>
#else

#endif

extern Object DYLIB_CLASS;
extern Value DYLIB_CLASS_NAME;

typedef struct {
	void *lib;
} DyLibrary;


extern void dylib_class_init(Object*);

extern Value dylib_class_method_load_method(Object*, Value, Value*, size_t);


extern Value load_dylib(Object*, Value, Value*, size_t);