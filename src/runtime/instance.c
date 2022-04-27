#include "runtime/instance.h"

void src_manager_init(SourceManager *m) {
	m->n_sources = 0;
	m->c_sources = 10;
	m->sources = calloc(m->c_sources, sizeof(Source));
}

void src_manager_destroy(SourceManager *m) {
	if (m->sources != NULL)
		free(m->sources);
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


void instance_init(Instance *instance) {
	src_manager_init(&instance->src_manager);
	thread_pool_init(&instance->thread_pool);
}

void instance_destroy(Instance *instance) {
	thread_pool_destroy(&instance->thread_pool);
  src_manager_destroy(&instance->src_manager);
}