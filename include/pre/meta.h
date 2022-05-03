#pragma once

#include "pre/ast.h"

typedef struct {
	size_t n_elif;

	int has_else;

	AstNode if_node;
	AstNode *elif_nodes;
	AstNode else_node;
	
} MetaDataIf;
