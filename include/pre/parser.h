#pragma once

#include "pre/tok.h"
#include "ast.h"

typedef enum {
	ASSOCIATIVITY_LEFT_TO_RIGHT,
	ASSOCIATIVITY_RIGHT_TO_LEFT
} ASSOCIATIVITY;

typedef struct fz_precedence {
	size_t n_operators;
	ASSOCIATIVITY associativity;
	const char **operators;
} PRECEDENCE;

extern const PRECEDENCE PRECEDENCE_LIST[];
extern const int PRECEDENCE_LIST_LENGTH;

typedef struct fz_parser Parser;

struct fz_parser {

};

extern AstNodeListIterator* get_container_end(AstNodeListIterator *i);

extern void parse(AstNodeList *l, AstNodeListIterator *i, AstNodeListIterator *e);