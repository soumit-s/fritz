#pragma once

#include "common.h"
#include "pre/tok.h"

typedef enum {
	NODE_TYPE_UNKWOWN,
	NODE_TYPE_TOKEN,
	NODE_TYPE_BINARY_OPERATOR,
	NODE_TYPE_UNARY_OPERATOR,
	NODE_TYPE_CONTAINER,
	NODE_TYPE_KEYWORD,
	NODE_TYPE_CONT_CALL,
	NODE_TYPE_SEPARATOR,
} NODE_TYPE;

typedef struct fz_ast_node AstNode;

struct fz_ast_node {
	NODE_TYPE type;
	Token value;
	AstNode *children;
	size_t n_children;
	void *meta_data;
};

typedef struct fz_ast_node_stack AstNodeStack;

struct fz_ast_node_stack {
	
};


typedef struct fz_ast_node_list_iterator AstNodeListIterator;
typedef struct fz_ast_node_list AstNodeList;

struct fz_ast_node_list_iterator {
	AstNode value;
	AstNodeListIterator *next;
	AstNodeListIterator *prev;
};

struct fz_ast_node_list {
	AstNodeListIterator *start;
};

extern void cvt_tokens_to_nodes(Token*, size_t, AstNodeList*);

extern void ast_node_list_init(AstNodeList*);
extern void ast_node_list_destroy(AstNodeList*);
extern size_t ast_node_list_size(AstNodeList*);

extern AstNodeListIterator* ast_node_list_begin(AstNodeList*);
extern AstNodeListIterator* ast_node_list_end(AstNodeList*);
//extern AstNodeListIterator* ast_node_list_start(AstNodeList*);

extern void ast_node_list_remove(AstNodeListIterator *iter);
extern void ast_node_list_flush(AstNodeList*);

extern void ast_node_list_insert_after(AstNodeList*, AstNode, AstNodeListIterator*);


extern void ast_node_destroy(AstNode);
extern void ast_token_node_init(AstNode*, Token);

extern void ast_dump_node(AstNode, int indent);
