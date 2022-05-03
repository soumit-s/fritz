#pragma once

#include "obj.h"

extern void scope_create(Object*, Object*, Object*);
extern void scope_create_implicit(Object*, Object*);

extern ObjectProperty* scope_get_property(Object*, Value);