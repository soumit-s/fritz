#pragma once

#include "runtime/instance.h"
#include "pre/ast.h"
#include "runtime/bcode.h"

extern void instance_compile_node(Instance*, AstNode, BcodeBuffer*, ConstantPoolCreator*);




