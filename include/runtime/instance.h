#pragma once

#include "runtime/const_pool.h"
#include "runtime/bcode.h"
#include "runtime/block.h"
#include "runtime/thread.h"

typedef struct fz_instance Instance;

typedef struct fz_source_manager SourceManager;

// Used by the Instance to manage Source(s).
struct fz_source_manager {
	// Library Paths inside which to look for
	// modules and libraries.
	string *lpaths;
	size_t n_lpaths;

	Source *sources;
	size_t n_sources;
	size_t c_sources;
};

extern void src_manager_init(SourceManager*);
extern void src_manager_destroy(SourceManager*);

// Adds a new Source and returns the ID of the source.
// If ID is 0, then source addition was not successfull.
extern SOURCE_ID src_manager_add(SourceManager*, Source);

extern Source src_manager_get(SourceManager*, SOURCE_ID);
#define SRC_MANAGER_GET_PTR(m, sid) (&m.sources[sid-1]);

extern SOURCE_ID src_manager_load(SourceManager*, string path);

// Represents a VM instance.
struct fz_instance {
	SourceManager src_manager;
	ThreadPool thread_pool;
};

extern void instance_init(Instance*);
extern void instance_destroy(Instance*);
