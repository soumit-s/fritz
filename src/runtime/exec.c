#include "runtime/exec.h"
#include "runtime/bcode.h"
#include "runtime/const_pool.h"
#include "runtime/scope.h"
#include "runtime/stack.h"
#include "runtime/frame.h"
#include "runtime/obj.h"
#include "runtime/universal.h"
#include "runtime/func.h"
#include <stdio.h>


#define OPCODE_EQUALS(a, b) *((short*)a) == *((short*)b)

#define CYCLE_START() __start:
#define CYCLE_END() goto __start; __end:
#define CYCLE_INIT() __init:
#define CYCLE_UPDATE() __update:

#define PERFORM_OPERATION(ip, x, y, r) \
	if (OPCODE_EQUALS(ip, OPCODE_ADD)) { r = x + y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_SUB)) { r = x - y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_MUL)) { r = x * y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_DIV)) { r = x / y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_MOD)) { r = (FzInt)x % (FzInt)y; } \

void instance_exec(Instance *i) {
	// Create the universal scope.
	object_init(&UNIVERSAL_SCOPE);
	universal_scope_create(&UNIVERSAL_SCOPE);

	Thread m_thread;
	thread_init(&m_thread);
	m_thread.id = 0;

	Source src = i->src_manager.sources[0];
	for(size_t i=0; i < src.const_pool.n_consts; ++i) {
		printf("%d. ", (int)i+1);
		value_log(src.const_pool.consts[i]);
		printf("\n");
	}
	printf("-----------");
	
	Block block;
	block.start_ptr = src.bcode.buffer;
	block.end_ptr = src.bcode.buffer + src.bcode.length;
	block.source = &(i->src_manager.sources[0]);

	Frame frame;
	frame_init(&frame);

	frame.scope = scope_create(NULL, NULL);
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

		if (OPCODE_EQUALS(ip, OPCODE_ADD) || OPCODE_EQUALS(ip, OPCODE_SUB)
			|| OPCODE_EQUALS(ip, OPCODE_MUL) || OPCODE_EQUALS(ip, OPCODE_DIV)
			|| OPCODE_EQUALS(ip, OPCODE_MOD)) {
			// Pop the last two values from the stack.
			Value v2 = STACK_POP(thread.stack);
			Value v1 = STACK_POP(thread.stack);

			// Both of them must be numbers.
			// (OPTIMIZABLE)
			Value r;

			if (v1.type == VALUE_TYPE_INT && v2.type == VALUE_TYPE_INT) {
				r.type = VALUE_TYPE_INT;
				PERFORM_OPERATION(ip, v1.i_value, v2.i_value, r.i_value);
			} else if (v1.type == VALUE_TYPE_FLOAT && v2.type == VALUE_TYPE_FLOAT) {
				r.type = VALUE_TYPE_FLOAT;
				PERFORM_OPERATION(ip, v1.f_value, v2.i_value, r.f_value);
			} else if (v1.type == VALUE_TYPE_INT && v2.type == VALUE_TYPE_FLOAT) {
				r.type = VALUE_TYPE_FLOAT;
				PERFORM_OPERATION(ip, v1.i_value, v2.f_value, r.f_value);
			} else if (v1.type == VALUE_TYPE_FLOAT && v2.type == VALUE_TYPE_INT) {
				r.type = VALUE_TYPE_FLOAT;
				PERFORM_OPERATION(ip, v1.f_value, v2.f_value, r.f_value);
			} else {
				// error.
				printf("fatal error: invalid operand types %d, %d\n", v1.type, v2.type);
				return;
			}

			STACK_PUSH(thread.stack, r);

			printf("add\n");
			ip += OPCODE_SIZE;
		} else if (OPCODE_EQUALS(ip, OPCODE_GET_SCOPE)) {
			
			printf("get.scope\n");
			CONSTANT_ID cid = *(CONSTANT_ID*)(ip += OPCODE_SIZE);

			Value key = constant_pool_get_value(curr_src.const_pool, cid);

			ObjectProperty *p;
			if ((p = scope_get_property(curr_scope, key)) == NULL) {
				// throw error that property does not exist.
				printf("property '");
				value_log(key);
				printf("' does not exist in object ");
				object_log(curr_scope);
				printf("\n");
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

			Value c = constant_pool_get_value(curr_src.const_pool, cid);

			// Push the constant value onto the stack.
			STACK_PUSH(thread.stack, c);

			ip += sizeof(CONSTANT_ID);

		} else if (OPCODE_EQUALS(ip, OPCODE_SET_SCOPE)) {

			printf("set.scope\n");
			Value key = constant_pool_get_value(curr_src.const_pool, *(CONSTANT_ID*)(ip += OPCODE_SIZE));

			ObjectProperty *p;
			if ((p = scope_get_property(curr_scope, key)) == NULL) {
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
			// Get the number of parameters.
			size_t n_params = (size_t)(*(FzInt*)(ip + OPCODE_SIZE));

			// Get the function.
			Value func = STACK_GET_FROM_TOP(thread.stack, n_params-1);
			if (func.type != VALUE_TYPE_OBJECT) {
				printf("fatal error: value ");
				value_log(func);
				printf(" is not callable.\n");
				return;
			}

			// TODO: Check whether the object is of type callable.

			// Check if function is a native function.
			if (func_get_native(func.o_ptr)) {
				Block func_block = func_get_block(func.o_ptr);
				NativeFunc f = (NativeFunc)func_block.start_ptr;

				f(thread.stack.top_ptr - n_params, n_params);
			}

			ip += OPCODE_SIZE + sizeof(FzInt);
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