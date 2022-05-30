#pragma once

#include "runtime/bcode.h"
#include "runtime/const_pool.h"

extern void bcode_dump(Bcode*, ConstantPool*);

// Converts module style path to
// os style path.
extern string mpath_to_opath(string, int*);