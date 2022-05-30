#include "runtime/instance.h"
#include "pre/ast.h"
#include "pre/parser.h"
#include "pre/tok.h"
#include "runtime/bcode.h"
#include "runtime/block.h"
#include "runtime/compiler.h"
#include "runtime/const_pool.h"
#include <stdio.h>

void src_manager_init(SourceManager *m) {
	m->n_sources = 0;
	m->c_sources = 10;
	m->sources = calloc(m->c_sources, sizeof(Source));
	m->n_lpaths = 0;
	m->lpaths = NULL;
}

void src_manager_destroy(SourceManager *m) {
	if (m->sources != NULL) {
		for (size_t i=0; i < m->n_sources; ++i) {
			Source src = m->sources[i];
			// Destroy the source code buffer.
			free(src.buffer);
			// Destroy the bytecode buffer
			free((uint8_t*)src.bcode.buffer);
			// Destroy the constant pool
			constant_pool_destroy(&src.const_pool);
		}
		free(m->sources);
	}
}

SOURCE_ID src_manager_add(SourceManager *m, Source s) {
	if (m->n_sources == m->c_sources) {
		size_t n_cap = m->c_sources + 10;
		Source *srcs = malloc(n_cap * sizeof(Source));
		memcpy(srcs, m->sources, m->c_sources * sizeof(Source));
		free(m->sources);
		m->sources = srcs;
		m->c_sources = n_cap;
	}

	s.id = m->n_sources + 1;
	m->sources[m->n_sources++] = s;

	return s.id; 
}

int src_manager_has(SourceManager *m, SOURCE_ID id) {
	return m->n_sources < id;
}

Source src_manager_get(SourceManager *m, SOURCE_ID id) {
	return m->sources[id-1];
}

SOURCE_ID src_manager_load(SourceManager *m, string path) {
	for (size_t i=0; i < m->n_lpaths; i++) {
		string lpath = m->lpaths[i];
		// Join the two paths
		char* p = calloc(1, lpath.length + path.length + 2);
		memcpy(p, lpath.value, lpath.length);
		p[lpath.length] = '/';
		memcpy(p + lpath.length + 1, path.value, path.length);

		// Try to open the file
		FILE *f = fopen(p, "r");
		if (!f) continue;

		size_t start = ftell(f);
		fseek(f, 0, SEEK_END);
		size_t end = ftell(f);
		fseek(f, 0, SEEK_SET);

		size_t bsize = end-start;

		char *buffer = malloc(bsize * sizeof(char));
		bsize = fread(buffer, sizeof(char), bsize, f);

		// Compile the code
		size_t n_tokens;
		Token *tokens = tokenize(buffer, bsize, &n_tokens);

		AstNodeList nodes;
		ast_node_list_init(&nodes);
		cvt_tokens_to_nodes(tokens, n_tokens, &nodes);

		parse(&nodes, ast_node_list_begin(&nodes), NULL);

		for (AstNodeListIterator *iter=ast_node_list_begin(&nodes)
					; iter != NULL; iter=iter->next)
			ast_dump_node(iter->value, 0);

		BcodeBuffer bcode_buff;
		bcode_buffer_init(&bcode_buff);

		ConstantPoolCreator pool_creator;
		constant_pool_creator_init(&pool_creator);

		AstNodeListIterator *iter = ast_node_list_begin(&nodes);
		for(;iter != NULL; iter=iter->next) {
			instance_compile_node(NULL, iter->value, &bcode_buff, &pool_creator);
		}

		Bcode bcode;
		bcode_buffer_to_bcode(&bcode_buff, &bcode);

		Source src;
		src.buffer = buffer;
		src.buffer_size = bsize;
		src.bcode = bcode;
		constant_pool_create(&src.const_pool, &pool_creator);

		src.id = src_manager_add(m, src);

		constant_pool_creator_destroy(&pool_creator);
		bcode_buffer_destroy(&bcode_buff);

		//free(buffer);

		// Free the path.
		free(p);

		fclose(f);

		return src.id;
	}

	return 0;
}


void instance_init(Instance *instance) {
	src_manager_init(&instance->src_manager);
	thread_pool_init(&instance->thread_pool);
}

void instance_destroy(Instance *instance) {
	thread_pool_destroy(&instance->thread_pool);
  src_manager_destroy(&instance->src_manager);
}