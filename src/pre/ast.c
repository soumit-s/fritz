#include "pre/ast.h"
#include "pre/meta.h"
#include "pre/tok.h"
#include <stdio.h>


void ast_token_node_init(AstNode *n, Token t) {
	n->type = NODE_TYPE_TOKEN;
	n->n_children = 0;
	n->children = NULL;
	n->meta_data = NULL;
	n->value = t;
}

void ast_node_list_init(AstNodeList *l) {
	l->start = calloc(1, sizeof(AstNodeListIterator));
}

/*AstNodeListIterator* ast_node_list_start(AstNodeList *l) {
	return l->start;
}*/

AstNodeListIterator* ast_node_list_begin(AstNodeList *l) {
	return l->start->next;
}

AstNodeListIterator* ast_node_list_end(AstNodeList *l) {
	AstNodeListIterator *iter = ast_node_list_begin(l);
	while (iter != NULL && iter->next != NULL) {
		iter = iter->next;
	}
	return iter;
}

void ast_node_list_flush(AstNodeList *l) {
	for (AstNodeListIterator *iter = ast_node_list_begin(l); iter != NULL;) {
		AstNodeListIterator *next = iter->next;
		free(iter);
		iter = next;
	}
}

void ast_node_list_destroy(AstNodeList *l) {
	ast_node_list_flush(l);
	free(l->start);
}

void ast_node_list_remove(AstNodeListIterator *iter) {
	iter->prev->next = iter->next;
	if (iter->next != NULL) {
		iter->next->prev = iter->prev;
	}
	free(iter);
}

size_t ast_node_list_size(AstNodeList *l) {
	size_t s = 0;
	for (AstNodeListIterator *iter = ast_node_list_begin(l); l != NULL; iter = iter->next, ++s);
	return s;
}

void ast_node_list_insert_after(AstNodeList *l, AstNode n, AstNodeListIterator *i) {
	if (i == NULL) {
		i = l->start;
	}

	AstNodeListIterator *t = calloc(1, sizeof(AstNodeListIterator));
	t->value = n;
	t->next = i->next;
	t->prev = i;

	i->next = t;
	if (t->next != NULL) {
		t->next->prev = t;
	}
}

void cvt_tokens_to_nodes(Token *tokens, size_t n_tokens, AstNodeList *l) {
	for (size_t i=n_tokens; i > 0;) {
		Token t = tokens[--i];
		if (t.type == TOKEN_TYPE_SPACE || t.type == TOKEN_TYPE_NEWLINE) {
			continue;
		}

		AstNode n;
		ast_token_node_init(&n, t);
		ast_node_list_insert_after(l, n, NULL);
	}
}


void ast_node_destroy(AstNode n) {
	if (n.children != NULL) {
		for (size_t i = n.n_children; i > 0;) {
			ast_node_destroy(n.children[--i]);
		}
		free(n.children);
	}
}

void _print_spaces(size_t n) {
	for (size_t i=0; i < n; ++i)
		printf("  ");
}

void ast_dump_node(AstNode n, int indent) {
	_print_spaces(indent);

	if (n.type == NODE_TYPE_CONT_CALL) {
		printf("<func_call>\n");
	} else {	
		for (size_t i=0; i < n.value.value.length; ++i)
			printf("%c", n.value.value.value[i]);
		//printf(" %d", n.type == NODE_TYPE_TOKEN);
		printf("\n");
	}

	if (n.type == NODE_TYPE_KEYWORD && 
			string_eqc(n.value.value, "if")) {
		MetaDataIf m = *(MetaDataIf*)n.meta_data;
		ast_dump_node(m.if_node, indent+1);

		for (size_t k=0; k < m.n_elif; ++k)
			ast_dump_node(m.elif_nodes[k], indent+1);

		if (m.has_else)
			ast_dump_node(m.else_node, indent+1);
	} 

	if (n.children != NULL) {
		for (size_t i=0; i < n.n_children; ++i) {
			ast_dump_node(n.children[i], indent+1);
		}
	}
}