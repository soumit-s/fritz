#pragma once

#include "runtime/instance.h"
#include "pre/ast.h"
#include "runtime/bcode.h"

extern void instance_compile_nodes(Instance *i, AstNode *nodes, size_t n_nodes,
                            BcodeBuffer *b, ConstantPoolCreator *p);

extern void instance_compile_node(Instance*, AstNode, BcodeBuffer*, ConstantPoolCreator*);




