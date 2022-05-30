#include "runtime/thread.h"
#include "runtime/fstack.h"


void thread_init(Thread *t) {
	t->status = THREAD_STATUS_INFANT;

	// The default size of the stack is 1024 kB 
	stack_create(&t->stack, 1024 * 1024 / sizeof(Value));

	// The default frame stack size is 512 kB
	frame_stack_create(&t->fstack, 512 * 1024 / sizeof(Frame));
}

void thread_destroy(Thread *t) {
	frame_stack_destroy(&t->fstack);
	stack_destroy(&t->stack);
}

void thread_iterator_init(ThreadIterator *i) {
	i->next = NULL;
}

void thread_pool_init(ThreadPool *p) {
	p->start = NULL;
	p->iter = NULL;
}

void thread_pool_destroy(ThreadPool *p) {
	ThreadIterator *start = thread_pool_iter(p);
	ThreadIterator *iter = start->next;
	while (iter != start) {
		ThreadIterator *next = iter->next;
		
		thread_destroy(&iter->thread);
		free(iter);

		iter = next;
	}

	thread_destroy(&start->thread);
	free(start);
}

void thread_pool_add(ThreadPool *p, Thread t) {
	ThreadIterator *t_iter = calloc(1, sizeof(ThreadIterator));
	ThreadIterator *p_iter = p->iter;
	thread_iterator_init(t_iter);
	t_iter->thread = t;

	if (p_iter != NULL) {
		t_iter->next = p_iter->next;
		p_iter->next = t_iter;
	} else {
		p->iter = t_iter;
		t_iter->next = t_iter;
	}
}

ThreadIterator* thread_pool_iter(ThreadPool *p) {
	return p->iter;
}