#include "tests.h"
#include "pre/ast.h"
#include "pre/tok.h"
#include "pre/parser.h"

#include "runtime/compiler.h"
#include "runtime/const_pool.h"
#include "runtime/instance.h"
#include "runtime/exec.h"
#include "runtime/utils.h"

#include <stdio.h>


Token *tokens;
size_t n_tokens;

char *buffer;
size_t buffer_size;

AstNodeList *l;

int tests_init() {
	FILE *f = fopen("./res/4.fz", "r");
	if (!f) {
		printf("failed to load file ./res/4.txt");
		return 1;
	}

	size_t s = ftell(f);
	fseek(f, 0, SEEK_END);
	s = ftell(f) - s;
	fseek(f, 0, SEEK_SET);

	buffer = malloc(sizeof(char) * s);
	fread(buffer, sizeof(char), s, f);
	fclose(f);

	buffer_size = s;
	printf("bytes read: %ld\n", buffer_size);
	return 0;
}

int test_tokenizer() {
	size_t c;
	tokens = tokenize(buffer, buffer_size, &c);
	if (!tokens) {
		return -1;
	}
	n_tokens = c;
	/*printf("num tokens read: %ld\n", c);
	for (int i=0; i < c; ++i) {
		Token token = tokens[i];
		token_info(token);
	}*/
	return 0;
}

int test_parser() {
	l = calloc(1, sizeof(AstNodeList));
	ast_node_list_init(l);
	cvt_tokens_to_nodes(tokens, n_tokens, l);

	/*for (AstNodeListIterator *i=ast_node_list_begin(&l); i != NULL; i = i->next) {
		ast_dump_node(i->value, 0);
	}*/

	/*AstNodeListIterator *i = get_container_end(ast_node_list_begin(&l));

	if (i == NULL) {
		printf("failed to find end\n");
		return -1;
	}

	printf("---------\n");
	token_info(i->value.value);*/

	parse(l, ast_node_list_begin(l), NULL);

	for (AstNodeListIterator *i=ast_node_list_begin(l); i != NULL; i=i->next)
		ast_dump_node(i->value, 0);

	return 0;
}

int test_compiler() {
	ConstantPoolCreator *pc = calloc(1, sizeof(ConstantPoolCreator));
	constant_pool_creator_init(pc);

	BcodeBuffer *b = calloc(1, sizeof(BcodeBuffer));
	Bcode *bc = calloc(1, sizeof(Bcode));
	bcode_buffer_init(b);
	
	for (AstNodeListIterator *i = ast_node_list_begin(l); i != NULL; i = i->next)
		instance_compile_node(NULL, i->value, b, pc);

	bcode_buffer_to_bcode(b, bc);

	for (size_t i=0; i < bc->length; ++i) 
		printf("%x ", bc->buffer[i]);
	printf("\n");

	bcode_dump(bc, NULL);

	Instance *instance = calloc(1, sizeof(Instance));
	instance_init(instance);

	ConstantPool p;
	constant_pool_create(&p, pc);

	Source src;
	src.buffer = buffer;
	src.buffer_size = buffer_size;
	src.bcode = *bc;
	src.const_pool = p;

	// Add the main source file.
	src_manager_add(&instance->src_manager, src);

	printf("-----------------------------------\n");

	string k = to_string("res");

	instance->src_manager.lpaths = &k;
	instance->src_manager.n_lpaths = 1;

	// Create the main thread to exectute the bytecode.
	instance_exec(instance);
	

	instance_destroy(instance);

	free(instance);

	free(b);
	free(pc);

	//free((uint8_t*)bc->buffer);
	free(bc);
	return 0;	
}

int tests_over() {
	if (l != NULL) {
		ast_node_list_destroy(l);
	}
	free(tokens);
	//free(buffer);
	return 0;
}