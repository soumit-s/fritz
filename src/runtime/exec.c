#include "runtime/exec.h"
#include "runtime/bcode.h"
#include "runtime/const_pool.h"
#include "runtime/stack.h"
#include "runtime/frame.h"
#include "runtime/obj.h"
#include <stdio.h>


#define OPCODE_EQUALS(a, b) *((short*)a) == *((short*)b)

#define CYCLE_START() __start:
#define CYCLE_END() goto __start; __end:
#define CYCLE_INIT() __init:
#define CYCLE_UPDATE() __update:

void instance_exec(Instance *i) {

	Thread m_thread;
	thread_init(&m_thread);
	m_thread.id = 0;

	Source src = i->src_manager.sources[0];
	
	Block block;
	block.start_ptr = src.bcode.buffer;
	block.end_ptr = src.bcode.buffer + src.bcode.length;
	block.source = &(i->src_manager.sources[0]);

	Frame frame;
	frame_init(&frame);

	frame.base_ptr = m_thread.stack.top_ptr;
	frame.block = block;
	frame.ip = block.start_ptr;

	FRAME_STACK_PUSH(m_thread.fstack, frame);

	thread_pool_add(&i->thread_pool, m_thread);

	// Start execution.
	instance_exec_threads(i);
}


void instance_exec_threads(Instance *i) {

	ThreadIterator *thread_iter = thread_pool_iter(&i->thread_pool);

	// The Source from which the block is being executed.
	Source curr_src;

	// Active scope.
	Object *curr_scope;

	// Active Frame.
	Frame *curr_frame;

	// Stores the address of the current instruction.
	const uint8_t *ip;

	// Stores the end address of the current source buffer.
	const uint8_t *ep;

	Thread thread;

	CYCLE_INIT()
		thread = thread_iter->thread;
		curr_frame = FRAME_STACK_GET_PTR(thread.fstack);
		curr_src = *(curr_frame->block.source);
		curr_scope = curr_frame->scope;
		ip = curr_frame->ip;
		ep = curr_frame->block.end_ptr;

	CYCLE_START() {

		if (ip >= ep) {
			goto __end;
		}

		if (OPCODE_EQUALS(ip, OPCODE_ADD) || OPCODE_EQUALS(ip, OPCODE_DIV)) {
			// Pop the last two values from the stack.
			printf("add\n");
			ip += OPCODE_SIZE;
		} else if (OPCODE_EQUALS(ip, OPCODE_GET_SCOPE)) {
			
			printf("get.scope\n");
			CONSTANT_ID cid = *(CONSTANT_ID*)(ip += OPCODE_SIZE);

			Value key = constant_pool_get_value(curr_src.const_pool, cid);

			ObjectProperty *p;
			if ((p = object_get_property(curr_scope, key)) == NULL) {
				// throw error that property does not exist.
				printf("property does not exist\n");
				// TEMPORARY: Abort for now
				return;
			}

			// Push the value onto the stack.
			STACK_PUSH(thread.stack, p->value);


			ip += sizeof(CONSTANT_ID);

		} else if (OPCODE_EQUALS(ip, OPCODE_GET_CONSTANT)) {
			// Decrement the stack pointer to push 
			// the value onto the stack.
			printf("get.constant\n");

			CONSTANT_ID cid = *((CONSTANT_ID*)(ip += OPCODE_SIZE));

			// Push the constant value onto the stack.
			STACK_PUSH(thread.stack, constant_pool_get_value(curr_src.const_pool, cid));

			ip += sizeof(CONSTANT_ID);

		} else if (OPCODE_EQUALS(ip, OPCODE_SET_SCOPE)) {

			printf("set.scope\n");
			Value key = constant_pool_get_value(curr_src.const_pool, *(CONSTANT_ID*)(ip += OPCODE_SIZE));

			ObjectProperty *p;
			if ((p = object_get_property(curr_scope, key)) == NULL) {
				// Since the property create a new object property
				// with the given key and default options.
				ObjectProperty prop;
				object_property_init(&prop);

				prop.key = key;
				prop.value = STACK_POP(thread.stack);

				object_add_property(curr_scope, prop);
			} else {
				p->value = STACK_POP(thread.stack); 
			}

			ip += sizeof(CONSTANT_ID);
		} else if (OPCODE_EQUALS(ip, OPCODE_INVOKE_CONSTANT)) {

		} else {
			printf("fatal error: unknown opcode (%d %d)\n", ip[0], ip[1]);
			return;
		}

	CYCLE_UPDATE()

		if (FALSE) {
			// Change the thread to the next one before
			// proceeding to the next iteration.
			thread_iter = thread_iter->next;
		}

	} CYCLE_END()

	return;
}