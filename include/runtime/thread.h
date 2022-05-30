#pragma once

#include "common.h"
#include "runtime/block.h"
#include "runtime/frame.h"
#include "runtime/fstack.h"

typedef int THREAD_ID;
typedef struct fz_thread Thread;

typedef enum {
	THREAD_STATUS_INFANT,
	THREAD_STATUS_ACTIVE,
	THREAD_STATUS_DEAD,
	THREAD_STATUS_UNKNOWN,
} THREAD_STATUS;


struct fz_thread {
	// The thread ID.
	THREAD_ID id;

	// Elements definig the execution state of a thread
	// is :- 
	// 1. The execution stack frame.
	// 2.	The execution scope.
	// 3. The block that is being executed.
	// 4. Pointer to the the bytecode instruction
	//    that has to be executed.
	// 5. Each thread has its own stack.

	// Thread stack.
	Stack stack;

	// Frame stack.
	FrameStack fstack;

	// Thread Status.
	THREAD_STATUS status;
};

extern void thread_init(Thread*);
extern void thread_destroy(Thread*);

typedef struct fz_thread_iterator ThreadIterator;

struct fz_thread_iterator {
	ThreadIterator *next;
	Thread thread;
};

typedef struct fz_thread_pool ThreadPool;

struct fz_thread_pool {
	ThreadIterator *start;
	ThreadIterator *iter;
};

extern void thread_pool_init(ThreadPool*);
extern void thread_pool_destroy(ThreadPool*);

extern void thread_pool_add(ThreadPool*, Thread);
extern ThreadIterator* thread_pool_iter(ThreadPool*);