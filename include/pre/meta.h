#pragma once

#include "pre/ast.h"

typedef struct {
	size_t n_elif;

	int has_else;

	AstNode if_node;
	AstNode *elif_nodes;
	AstNode else_node;
	
} MetaDataIf;

typedef struct {
	// Whether the detach operator has been used on the
	// funcion call.
	int detached;

} MetaDataContCall;

#define META_DATA_CONT_CALL_NEW() {.detached=FALSE}
