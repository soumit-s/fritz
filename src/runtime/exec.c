#include "runtime/exec.h"
#include "runtime/bcode.h"
#include "runtime/block.h"
#include "runtime/const_pool.h"
#include "runtime/scope.h"
#include "runtime/stack.h"
#include "runtime/frame.h"
#include "runtime/obj.h"
#include "runtime/universal.h"
#include "runtime/func.h"
#include "runtime/class.h"
#include <stdio.h>

#define DEBUG

#define OPCODE_EQUALS(a, b) *((short*)a) == *((short*)b)

#define CYCLE_START() __start:
#define CYCLE_END() goto __start; __end:
#define CYCLE_INIT() __init:
#define CYCLE_UPDATE() __update:

#define CYCLE_STAGE_FETCH_CURRENT_FRAME() __current_frame_fetch:
#define JUMP_TO_STAGE_FETCH_CURRENT_FRAME() goto __current_frame_fetch

#define CYCLE_STAGE_CURRENT_FRAME_DEPS_INIT() __current_frame_deps_init:
#define JUMP_TO_STAGE_CURRENT_FRAME_DEPS_INIT() goto __current_frame_deps_init

#define PERFORM_OPERATION(ip, x, y, r) \
	if (OPCODE_EQUALS(ip, OPCODE_ADD)) { r = x + y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_SUB)) { r = x - y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_MUL)) { r = x * y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_DIV)) { r = x / y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_MOD)) { r = (FzInt)x % (FzInt)y; } \

#define PERFORM_RELATIONAL_OPERATION(ip, x, y, r) \
	if (OPCODE_EQUALS(ip, OPCODE_GT)) { r = x > y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_LT)) { r = x < y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_GTE)) { r = x >= y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_LTE)) { r = x <= y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_EQ)) { r = x == y; } \
	else if (OPCODE_EQUALS(ip, OPCODE_NEQ)) { r = x != y; }


void print_stack(Stack stack) {
	Value *e = stack.top_ptr, *i = stack.values;
	for (;--e >= i;) {
		value_log(*e);
		printf("\n");
	}
}

void instance_exec(Instance *i) {
	// Create the universal scope.
	object_init(&UNIVERSAL_SCOPE);
	universal_scope_create(&UNIVERSAL_SCOPE);

	Thread m_thread;
	thread_init(&m_thread);
	m_thread.id = 0;

	Source src = i->src_manager.sources[0];

	#ifdef DEBUG
	for(size_t i=0; i < src.const_pool.n_consts; ++i) {
		printf("%d. ", (int)i);
		value_log(src.const_pool.consts[i]);
		printf("\n");
	}
	printf("--------------------\n");
	#endif
	
	Block block;
	block.type = BLOCK_TYPE_EXPLICIT;
	block.start_ptr = src.bcode.buffer;
	block.end_ptr = src.bcode.buffer + src.bcode.length;
	block.source = &(i->src_manager.sources[0]);

	Frame frame;
	frame_init(&frame);

	Object scope;
	object_init(&scope);
	scope_create(&scope, NULL, NULL);

	frame.read_scope = &scope;
	frame.write_scope = &scope;
	frame.base_ptr = m_thread.stack.top_ptr;
	frame.block = block;
	frame.ip = block.start_ptr;

	FRAME_STACK_PUSH(m_thread.fstack, frame);

	thread_pool_add(&i->thread_pool, m_thread);

	// Start execution.
	instance_exec_threads(i);
}

void instance_exec_threads(Instance *i) {

	// Prequisites
	Value NULL_VALUE;
	NULL_VALUE.type = VALUE_TYPE_OBJECT;
	NULL_VALUE.o_ptr = NULL;

	ThreadIterator *thread_iter = thread_pool_iter(&i->thread_pool);

	// The Source from which the block is being executed.
	Source curr_src;

	// Active scope.
	Object *read_scope, *write_scope;

	// Active Frame.
	Frame *curr_frame;

	// Stores the address of the current instruction.
	const uint8_t *ip;

	// Stores the end address of the current source buffer.
	const uint8_t *ep;

	Thread thread;

	CYCLE_INIT()
		thread = thread_iter->thread;

		CYCLE_STAGE_FETCH_CURRENT_FRAME()
			curr_frame = FRAME_STACK_GET_PTR(thread.fstack);

		CYCLE_STAGE_CURRENT_FRAME_DEPS_INIT()
			curr_src = *(curr_frame->block.source);
			read_scope = curr_frame->read_scope;
			write_scope = curr_frame->write_scope;
			ip = curr_frame->ip;
			ep = curr_frame->block.end_ptr;

	CYCLE_START() {

		if (ip >= ep) {
			switch (curr_frame->block.type) {
			case BLOCK_TYPE_EXPLICIT:

				break;
			case BLOCK_TYPE_IMPLICIT:
				// Readjust the stack.
				STACK_SET_TOP(thread.stack, curr_frame->base_ptr);

				// Push the write scope onto the stack.
				STACK_PUSH(thread.stack, value_object(write_scope));

				break;

			case BLOCK_TYPE_NOEXEC: {
					// These kind of blocks must return a value 
					// when executed. Pop the topmost element from
					// the stack to get the return value.
					Value v = STACK_POP(thread.stack);
					STACK_SET_TOP(thread.stack, curr_frame->base_ptr);
					STACK_PUSH(thread.stack, v);

					break;
				}	
			}

			// Readjust the call frame stack..
			FRAME_STACK_POP(thread.fstack);

			// If there are no frames left in the
			// frame stack, then exit the thread.
			if (thread.fstack.top_ptr == thread.fstack.frames) {
				goto __end;
			}

			JUMP_TO_STAGE_FETCH_CURRENT_FRAME();
		}

		#ifdef DEBUG
			printf("\033[1;32m%s\033[m\n", bcode_opcode_to_inst(ip));
		#endif

		if (OPCODE_EQUALS(ip, OPCODE_POP)) {
			thread.stack.top_ptr--;
			ip += OPCODE_SIZE;

		} else if (OPCODE_EQUALS(ip, OPCODE_ADD) || OPCODE_EQUALS(ip, OPCODE_SUB)
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

			ip += OPCODE_SIZE;

		} else if (OPCODE_EQUALS(ip, OPCODE_GT) || OPCODE_EQUALS(ip, OPCODE_LT)
				|| OPCODE_EQUALS(ip, OPCODE_GTE) || OPCODE_EQUALS(ip, OPCODE_LTE)
				|| OPCODE_EQUALS(ip, OPCODE_EQ) || OPCODE_EQUALS(ip, OPCODE_NEQ)) {
			
			Value v2 = STACK_POP(thread.stack);
			Value v1 = STACK_POP(thread.stack);

			Value r;
			r.type = VALUE_TYPE_BOOLEAN;

			if (v1.type == VALUE_TYPE_INT && v2.type == VALUE_TYPE_INT) {
				PERFORM_RELATIONAL_OPERATION(ip, v1.i_value, v2.i_value, r.b_value);
			} else if (v1.type == VALUE_TYPE_FLOAT && v2.type == VALUE_TYPE_FLOAT) {
				PERFORM_RELATIONAL_OPERATION(ip, v1.f_value, v2.i_value, r.b_value);
			} else if (v1.type == VALUE_TYPE_INT && v2.type == VALUE_TYPE_FLOAT) {
				PERFORM_RELATIONAL_OPERATION(ip, v1.i_value, v2.f_value, r.b_value);
			} else if (v1.type == VALUE_TYPE_FLOAT && v2.type == VALUE_TYPE_INT) {
				PERFORM_RELATIONAL_OPERATION(ip, v1.f_value, v2.f_value, r.b_value);
			} else {
				// error.
				printf("fatal error: invalid operand types %d, %d\n", v1.type, v2.type);
				return;
			}

			STACK_PUSH(thread.stack, r);

			ip += OPCODE_SIZE;

		} else if (OPCODE_EQUALS(ip, OPCODE_GET_SCOPE)) {
			
			CONSTANT_ID cid = *(CONSTANT_ID*)(ip += OPCODE_SIZE);

			Value key = constant_pool_get_value(curr_src.const_pool, cid);

			ObjectProperty *p;
			if ((p = scope_get_property(read_scope, key)) == NULL) {
				// throw error that property does not exist.
				printf("property '");
				value_log(key);
				printf("' does not exist in object ");
				object_log(read_scope);
				printf("\n");
				// TEMPORARY: Abort for now
				return;
			}

			// Push the value onto the stack.
			STACK_PUSH(thread.stack, p->value);


			ip += sizeof(CONSTANT_ID);

		} else if (OPCODE_EQUALS(ip, OPCODE_GET_CONSTANT)) {
			CONSTANT_ID cid = *((CONSTANT_ID*)(ip += OPCODE_SIZE));

			Value c = constant_pool_get_value(curr_src.const_pool, cid);

			// Push the constant value onto the stack.
			STACK_PUSH(thread.stack, c);

			ip += sizeof(CONSTANT_ID);

		} else if (OPCODE_EQUALS(ip, OPCODE_GET_NULL)) {
			STACK_PUSH(thread.stack, NULL_VALUE);
			ip += OPCODE_SIZE;

		} else if (OPCODE_EQUALS(ip, OPCODE_SET_SCOPE)) {

			Value key = constant_pool_get_value(curr_src.const_pool, *(CONSTANT_ID*)(ip += OPCODE_SIZE));

			ObjectProperty *p;
			if ((p = scope_get_property(write_scope, key)) == NULL) {
				// Since the property create a new object property
				// with the given key and default options.
				ObjectProperty prop;
				object_property_init(&prop);

				prop.key = key;
				prop.value = STACK_POP(thread.stack);

				object_add_property(write_scope, prop);
			} else {
				p->value = STACK_POP(thread.stack); 
			}

			ip += sizeof(CONSTANT_ID);

		} else if (OPCODE_EQUALS(ip, OPCODE_SET_SCOPE_STACK)) {
			Value key, value;
			key = STACK_POP(thread.stack);
			value = STACK_POP(thread.stack);

			// TODO: Check if the property is writable

			// Write the value
			object_set_property_value(write_scope, key, value);

			ip += OPCODE_SIZE;

		} else if (OPCODE_EQUALS(ip, OPCODE_GET_OBJECT)) {
			// Pop the key.
			Value key = STACK_POP(thread.stack);
			// Pop the object.
			Value obj = STACK_POP(thread.stack);

			if(obj.type != VALUE_TYPE_OBJECT) {
				printf("fatal error: ");
				value_log(obj);
				printf(" is not an object\n");
				return;
			}

			// Null Value check
			if (!obj.o_ptr) {
				printf("fatal error: cannot look for property '");
				value_log(key);
				printf("' inside (null)\n");
				return;
			}

			ObjectProperty *p = object_get_property(obj.o_ptr, key);
			if (p == NULL) {
				printf("fatal error: property '");
				value_log(key);
				printf("' not found in object ");
				value_log(obj);
				printf("\n");
				return;
			}

			STACK_PUSH(thread.stack, p->value);

			ip += OPCODE_SIZE;

		} else if (OPCODE_EQUALS(ip, OPCODE_SET_OBJECT)) {

			Value k = STACK_POP(thread.stack);
			Value o = STACK_POP(thread.stack);
			Value v = STACK_POP(thread.stack);

			if (o.type != VALUE_TYPE_OBJECT || o.o_ptr == NULL) {
				if (o.type != VALUE_TYPE_OBJECT) {
					printf("fatal error:");
					value_log(o);
					printf(" is not an object\n");
				} else {
					printf("fatal error: cannot set the property of (null)\n");
				}
				return;
			}
			object_set_property_value(o.o_ptr, k, v);
			ip += OPCODE_SIZE;

		} else if (OPCODE_EQUALS(ip, OPCODE_BLOCK_NOEXEC_START)) {
			size_t offset = *((const size_t*)(ip + OPCODE_SIZE));

			Block bl;
			bl.type = BLOCK_TYPE_NOEXEC;
			bl.source = curr_frame->block.source;
			bl.start_ptr = ip + OPCODE_SIZE + sizeof(size_t);

			const uint8_t *e_addr = bl.start_ptr + offset;
			if (!(OPCODE_EQUALS(e_addr, OPCODE_BLOCK_NOEXEC_END))) {
				printf("fatal error: failed to find end of block.noexec (Very FISHY !)\n");
				return;
			}
			bl.end_ptr = e_addr;

			STACK_PUSH(thread.stack, value_block(bl));

			ip = e_addr + OPCODE_SIZE;

		} else if (OPCODE_EQUALS(ip, OPCODE_BLOCK_IMPLICIT_START)) {

			// Allocate a new object as the write scope.
			// This object is going to be pushed onto the stack
			// after the block has been executed.

			Object *obj = malloc(sizeof(Object));
			object_init(obj);

			size_t offset = *(const size_t*)(ip + OPCODE_SIZE);

			Block bl;
			bl.type = BLOCK_TYPE_IMPLICIT;
			bl.start_ptr = ip + OPCODE_SIZE + sizeof(size_t);
			bl.end_ptr = bl.start_ptr + offset;
			bl.source = curr_frame->block.source;

			// TODO: Check id end_ptr is valid.

			// Create the frame for executing the block
			Frame f;
			frame_init(&f);
			f.read_scope = read_scope;
			f.write_scope = obj;
			f.block = bl;
			f.base_ptr = thread.stack.top_ptr;
			f.ip = bl.start_ptr;

			// Store the ip of the instruction that
			// is to be executed after the block has been 
			// executed.
			curr_frame->ip = bl.end_ptr + OPCODE_SIZE;

			// TODO: Implement anti-(stack smashing and stack leak) code.
			FRAME_STACK_PUSH(thread.fstack, f);

			JUMP_TO_STAGE_FETCH_CURRENT_FRAME();

		} else if (OPCODE_EQUALS(ip, OPCODE_OBJECT_CREATE)) {


		} else if (OPCODE_EQUALS(ip, OPCODE_METHOD_CREATE)) {
			Value v_body = STACK_POP(thread.stack);
			Value v_proto= STACK_POP(thread.stack);

			Object *func = malloc(sizeof(Object));
			object_init(func);

			if (v_proto.type != VALUE_TYPE_OBJECT) {
				// error.
			}

			if (v_body.type != VALUE_TYPE_BLOCK) {
				// error.
			}

			func_create(func, v_proto.o_ptr, v_body.bl_value, read_scope);

			STACK_PUSH(thread.stack, value_object(func));

			ip += OPCODE_SIZE;

		} else if (OPCODE_EQUALS(ip, OPCODE_RETURN)) {
			ip = ep;
			
		} else if (OPCODE_EQUALS(ip, OPCODE_RETURN_NULL)) {
			STACK_PUSH(thread.stack, NULL_VALUE);
			ip = ep;

		} else if (OPCODE_EQUALS(ip, OPCODE_JUMP_IF)) {
			ip += OPCODE_SIZE + sizeof(size_t) + 
				(STACK_POP(thread.stack).b_value ? 
					*(const size_t*)(ip + OPCODE_SIZE) : 0);

		} else if (OPCODE_EQUALS(ip, OPCODE_JUMP_NIF)) {
			size_t offset = 0;
			if (!(STACK_POP(thread.stack).b_value)) {
				offset = *(size_t*)(ip + OPCODE_SIZE);
			} 
			
			ip += OPCODE_SIZE + sizeof(size_t) + offset;

		} else if (OPCODE_EQUALS(ip, OPCODE_JUMP_IF_NOPOP)) {
			ip += OPCODE_SIZE + sizeof(size_t) + 
				(STACK_GET_FROM_TOP(thread.stack, 1).b_value ? 
					*(const size_t*)(ip + OPCODE_SIZE) : 0);

		} else if (OPCODE_EQUALS(ip, OPCODE_JUMP_NIF_NOPOP)) {
			size_t offset = 0;
			if (!(STACK_GET_FROM_TOP(thread.stack, 1).b_value)) {
				offset = *(size_t*)(ip + OPCODE_SIZE);
			} 
			
			ip += OPCODE_SIZE + sizeof(size_t) + offset;

		} else if (OPCODE_EQUALS(ip, OPCODE_JUMP)) {
			ip += OPCODE_SIZE + sizeof(size_t) + 
				*(size_t*)(ip + OPCODE_SIZE);

		} else if (OPCODE_EQUALS(ip, OPCODE_JUMP_BACK)) {
			ip -= *(size_t*)(ip + OPCODE_SIZE);
			
		} else if (OPCODE_EQUALS(ip, OPCODE_INVOKE_CONSTANT)) {
			//print_stack(thread.stack);
			// Get the function.
			Value func = STACK_POP(thread.stack);
			if (func.type != VALUE_TYPE_OBJECT) {
				printf("fatal error: value ");
				value_log(func);
				printf(" is not callable.\n");
				return;
			}

			// Get the number of parameters.
			size_t n_args = (size_t)(*(FzInt*)(ip + OPCODE_SIZE));
			Value *args = STACK_GET_FROM_TOP_PTR(thread.stack, n_args);

			// TODO: Check whether the object is of type callable.

			// Check if function is a native function.
			if (func_get_native(func.o_ptr)) {
				Block func_block = func_get_block(func.o_ptr);
				NativeFunc f = (NativeFunc)func_block.start_ptr;

				Value r = f(args, n_args);

				// Erase the params and the function from the stack.
				STACK_SET_TOP(
					thread.stack,
					args);
				
				STACK_PUSH(thread.stack, r);

				ip += OPCODE_SIZE + sizeof(FzInt);
				
			} else {
				// For now only allow normal variables.
				Object *origin_scope = NULL;

				ObjectProperty *params_p;
				ObjectProperty *origin_p;

				params_p = object_get_property(func.o_ptr, value_string(to_string("params")));
				origin_p = object_get_property(func.o_ptr, value_string(to_string("scope")));
				
				if (origin_p && origin_p->value.type == VALUE_TYPE_OBJECT) {
					origin_scope = origin_p->value.o_ptr;
				}

				if (params_p && params_p->value.type == VALUE_TYPE_OBJECT) {

					// Create the new read and write.
					Object *scope = malloc(sizeof(Object));
					object_init(scope);
					scope_create(scope, origin_scope, NULL);

					ObjectProperty *params = params_p->value.o_ptr->props;
					size_t n_params = params_p->value.o_ptr->n_props;

					size_t n = n_params > n_args ? n_args : n_params;

					for (size_t k=0; k < n; ++k) {
						object_set_property_value(scope, params[k].key, args[k]);
					}

					// Create the new frame
					Frame f;
					f.block = func_get_block(func.o_ptr);

					// Erase the arguments along with the function
					// object itself from the stack.
					STACK_SET_TOP(thread.stack, args);

					f.base_ptr = thread.stack.top_ptr;
					f.read_scope = scope;
					f.write_scope = scope;
					f.ip = f.block.start_ptr;

					// Store the address of the instruction that
					// is to be executed after returning from the 
					// function.
					curr_frame->ip = ip + OPCODE_SIZE + sizeof(FzInt);

					// Push the frame onto the frame stack.
					FRAME_STACK_PUSH(thread.fstack, f);

					// Jump to the frame fetch stage.
					JUMP_TO_STAGE_FETCH_CURRENT_FRAME();
				}
			} 
		} else if (OPCODE_EQUALS(ip, OPCODE_STACK_DUP_TOP)) {
			STACK_PUSH(thread.stack, STACK_GET_FROM_TOP(thread.stack, 1));
			ip += OPCODE_SIZE;
		
		} else if (OPCODE_EQUALS(ip, OPCODE_CLASSIFY)) {

			// Pop the super, body and name.
			Value super, body, name;
			name = STACK_POP(thread.stack);
			body = STACK_POP(thread.stack);
			super = STACK_POP(thread.stack);

			// TODO: Both super and body must be objects.

			// Ignore super for now.
			Object *clazz = class_create(body.o_ptr, super.o_ptr, name);
			if (!clazz) {
				// error
				return;
			}

			// Reuse body for storing the clazz
			body.o_ptr = clazz;

			// Push back the name and the class
			STACK_PUSH(thread.stack, body);
			STACK_PUSH(thread.stack, name);

			ip += OPCODE_SIZE;
		} else {
			printf("fatal error: unknown opcode (0x%d 0x%d)\n", ip[0], ip[1]);
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