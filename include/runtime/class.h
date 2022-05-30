#pragma once

#include "runtime/obj.h"

extern Value CONSTRUCTOR_METHOD_NAME, SUPER_PROP_NAME, CLASS_NAME_PROP_NAME;

extern Object* class_create(Object *c, Object *s, Value n);

extern Object* class_instantiate(Object *c);