#pragma once

#include "obj.h"

extern Object* scope_create(Object*, Object*);

extern ObjectProperty* scope_get_property(Object*, Value);