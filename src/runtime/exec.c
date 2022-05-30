#include "runtime/exec.h"
#include "runtime/bcode.h"
#include "runtime/block.h"
#include "runtime/const_pool.h"
#include "runtime/instance.h"
#include "runtime/scope.h"
#include "runtime/stack.h"
#include "runtime/frame.h"
#include "runtime/obj.h"
#include "runtime/thread.h"
#include "runtime/universal.h"
#include "runtime/func.h"
#include "runtime/class.h"
#include "runtime/universal/bool.h"
#include "runtime/universal/float.h"
#include "runtime/universal/int.h"
#include "runtime/universal/method.h"
#include "runtime/universal/object.h"
#include "runtime/universal/list.h"
#include "runtime/universal/string.h"
#include "runtime/utils.h"
#include "pre/tok.h"
#include "runtime/class.h"
#include "str.h"
#include <stdio.h>

#define DEBUG

#define LOG_VALUE(v) \
	printf("\033[42m"); \
	value_log(v); \
	printf("\033[0m\n"); \

#define OPCODE_EQUALS(a, b) *((short*)a) == *((short*)b)

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


void payload_func_call_init(PAYLOAD_FUNC_CALL *p) {
	p->func = NULL;
	p->is_async = FALSE;
	p->is_me = FALSE;
	p->n_params = 0;
	p->params = NULL;
	p->rip = NULL;
	p->me = VALUE_NULL;
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
	bcode_dump(&src.bcode, &src.const_pool);
	printf("----------------------\n");
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

	// Define the labels for the opcode handler in the global
	// to access them outside the instance_exec_thread 
	// function.

	BEGIN_MAP()
		MAP(POP)
		MAP(ADD) MAP(SUB) MAP(MUL) MAP(DIV) MAP(MOD)
		MAP(GT) MAP(GTE) MAP(LT) MAP(LTE) MAP(EQ) MAP(NEQ)
		MAP(NOT) MAP(PROTO)
		MAP(GET_SCOPE)
		MAP(GET_CONSTANT)
		MAP(GET_NULL)
		MAP(SET_SCOPE)
		MAP(SET_SCOPE_STACK)
		MAP(GET_OBJECT)
		MAP(SET_OBJECT)
		MAP(BLOCK_NOEXEC_START)
		MAP(BLOCK_IMPLICIT_START)
		MAP(BLOCK_NOEXEC_IMPLICIT_START)
		MAP(OBJECT_CREATE)
		MAP(METHOD_CREATE)
		MAP(RETURN)
		MAP(RETURN_NULL)
		MAP(JUMP_IF)
		MAP(JUMP_NIF)
		MAP(JUMP_IF_NOPOP)
		MAP(JUMP_NIF_NOPOP)
		MAP(JUMP)
		MAP(JUMP_BACK)
		MAP(INVOKE_CONSTANT)
		MAP(INVOKE_CONSTANT_ASYNC)
		MAP(INVOKE_CONSTANT_ME)
		MAP(INVOKE_CONSTANT_ME_ASYNC)
		MAP(STACK_DUP_TOP)
		MAP(CLASSIFY)
		MAP(INSTANTIATE)
		MAP(MODULE_LOAD)
		MAP(SET_SCOPE_DEPENDANT_STACK)
		MAP(SET_OBJECT_DEPENDANT)
		MAP(LISTIFY)
		MAP(ATTACH_OBJECT) MAP(ATTACH_SCOPE)
	END_MAP()

	SUBMODULE_PAYLOADS PAYLOADS;
	payload_func_call_init(&PAYLOADS.FUNC_CALL);

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

	// Counts the number of instructions that have been
	// executed out of every 8 instruction.
	// Example: If 1 instruction has been executed then n_insts = 7
	int n_insts = 8;

	CYCLE_INIT()
		CYCLE_STAGE_THREAD_SELECT()
			thread = thread_iter->thread;
			if (thread.status == THREAD_STATUS_DEAD) {
				JUMP_TO_STAGE_NEXT_THREAD();
			}

		CYCLE_STAGE_FETCH_CURRENT_FRAME()
			curr_frame = FRAME_STACK_GET_PTR(thread.fstack);

		CYCLE_STAGE_CURRENT_FRAME_DEPS_INIT()
			curr_src = *(curr_frame->block.source);
			read_scope = curr_frame->read_scope;
			write_scope = curr_frame->write_scope;
			ip = curr_frame->ip;
			ep = curr_frame->block.end_ptr;

	CYCLE_START() {

		if (!n_insts) {
			n_insts = 8;
			JUMP_TO_STAGE_NEXT_THREAD();
		}

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
			
			case BLOCK_TYPE_NOEXEC_IMPLICT: {
				STACK_SET_TOP(thread.stack, curr_frame->base_ptr);
				break;
			}

			case BLOCK_TYPE_EXTERNAL: {
					STACK_SET_TOP(thread.stack, curr_frame->base_ptr);
					STACK_PUSH(thread.stack, value_object(write_scope));
				}
			}

			// Readjust the call frame stack..
			FRAME_STACK_POP(thread.fstack);

			// If there are no frames left in the
			// frame stack, then exit the thread.
			if (thread.fstack.top_ptr == thread.fstack.frames) {
				thread.status = THREAD_STATUS_DEAD;

				thread_iter->thread = thread;
				// If the thread is the main thread then
				// end execution. Otherwise goto cycle update
				// to update the thread.
				if (!thread.id) {
					goto __end;
				} else {
					JUMP_TO_STAGE_NEXT_THREAD();
				}
			}

			JUMP_TO_STAGE_FETCH_CURRENT_FRAME();
		}

		#ifdef DEBUG
			printf("\033[1;32m%s\033[m\n", bcode_opcode_to_inst(ip));
		#endif

		--n_insts;

		CHOOSE_HANDLER(ip);

		START_HANDLER(POP) {
			thread.stack.top_ptr--;
			ip += OPCODE_SIZE;

		} END_HANDLER()

		START_HANDLER(ADD) 
		START_HANDLER(SUB)
		START_HANDLER(MUL)
		START_HANDLER(DIV)
		START_HANDLER(MOD) {
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
			} else if (v1.type == VALUE_TYPE_STRING && v2.type == VALUE_TYPE_STRING
					&& OPCODE_EQUALS(ip, OPCODE_ADD)) {
				
				r.type = VALUE_TYPE_STRING;
				r.s_value.length = v1.s_value.length + v2.s_value.length;
				r.s_value.value = malloc(r.s_value.length * sizeof(char));
				memcpy((char*)r.s_value.value, v1.s_value.value, 
					v1.s_value.length);
				memcpy((char*)r.s_value.value + v1.s_value.length, v2.s_value.value, 
					v2.s_value.length);
			} else if (v1.type == VALUE_TYPE_STRING) {

			} else if (v2.type == VALUE_TYPE_STRING) {

			} else {
				// error.
				printf("fatal error: invalid operand types %d, %d\n", v1.type, v2.type);
				return;
			}

			STACK_PUSH(thread.stack, r);

			ip += OPCODE_SIZE;

		} END_HANDLER()


		START_HANDLER(GT) START_HANDLER(LT)
		START_HANDLER(GTE) START_HANDLER(LTE)
		START_HANDLER(EQ) START_HANDLER(NEQ) {
			
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
			} else if (v1.type == VALUE_TYPE_OBJECT && v2.type == VALUE_TYPE_OBJECT) {
				
				if (OPCODE_EQUALS(ip, OPCODE_EQ)) {
					r.b_value = v1.o_ptr == v2.o_ptr;
				} else if (OPCODE_EQUALS(ip, OPCODE_NEQ)) {
					r.b_value = v1.o_ptr != v2.o_ptr;
				} else {
					printf("fatal error: invalid operand types %d, %d\n", v1.type, v2.type);
					return;
				}

			} else {
				// error.
				printf("fatal error: invalid operand types %d, %d\n", v1.type, v2.type);
				return;
			}

			STACK_PUSH(thread.stack, r);

			ip += OPCODE_SIZE;

		} END_HANDLER()

		START_HANDLER(NOT) {
			Value v = STACK_POP(thread.stack);
			
			switch (v.type) {
				case VALUE_TYPE_OBJECT:
					STACK_PUSH(thread.stack, v.o_ptr ? VALUE_FALSE : VALUE_TRUE);
					break; 
				case VALUE_TYPE_STRING:
					STACK_PUSH(thread.stack, 
						v.s_value.length ? VALUE_FALSE : VALUE_TRUE);
					break;
				case VALUE_TYPE_BOOLEAN:
					v.b_value = !v.b_value;
					STACK_PUSH(thread.stack, v);
					break;
				case VALUE_TYPE_INT:
					STACK_PUSH(thread.stack, 
						v.i_value ? VALUE_FALSE : VALUE_TRUE);
					break;
				case VALUE_TYPE_FLOAT:
					STACK_PUSH(thread.stack,
						v.f_value == 0 ? VALUE_TRUE : VALUE_FALSE);
					break;
				default:
					STACK_PUSH(thread.stack, VALUE_TRUE);
			}

			ip += OPCODE_SIZE;

		} END_HANDLER()

		START_HANDLER(PROTO) {
			Value v = STACK_POP(thread.stack);
			switch (v.type) {
				case VALUE_TYPE_OBJECT:
					STACK_PUSH(thread.stack,
						v.o_ptr && v.o_ptr->blueprint ?
							value_object(v.o_ptr->blueprint) :
							value_object(&OBJECT_CLASS));
					break;
				case VALUE_TYPE_STRING:
					STACK_PUSH(thread.stack, 
						value_object(&STRING_CLASS));
					break;
				default:
					STACK_PUSH(thread.stack, 
						value_object(&OBJECT_CLASS));
			}

			ip += OPCODE_SIZE;

		} END_HANDLER()

		START_HANDLER(GET_SCOPE) {
			
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

			if (p->getter) {
				// If the property has a getter then call the getter.
				PAYLOADS.FUNC_CALL.func = p->getter;
				PAYLOADS.FUNC_CALL.is_async = FALSE;
				PAYLOADS.FUNC_CALL.is_me = TRUE;
				PAYLOADS.FUNC_CALL.me = value_object(read_scope);
				PAYLOADS.FUNC_CALL.n_params = 1;
				PAYLOADS.FUNC_CALL.params = &p->value;
				PAYLOADS.FUNC_CALL.rip = ip + sizeof(CONSTANT_ID);
				SUBMODULE_CALL(FUNC_CALL);
			} else {
				// Push the value onto the stack.
				STACK_PUSH(thread.stack, p->value);
			}

			ip += sizeof(CONSTANT_ID);

		} END_HANDLER() 

		START_HANDLER(GET_CONSTANT) {
			CONSTANT_ID cid = *((CONSTANT_ID*)(ip += OPCODE_SIZE));

			Value c = constant_pool_get_value(curr_src.const_pool, cid);

			// Push the constant value onto the stack.
			STACK_PUSH(thread.stack, c);

			ip += sizeof(CONSTANT_ID);

		} END_HANDLER()

		START_HANDLER(GET_NULL) {
			STACK_PUSH(thread.stack, NULL_VALUE);
			ip += OPCODE_SIZE;

		} END_HANDLER() 

		START_HANDLER(SET_SCOPE) {

			Value key = constant_pool_get_value(curr_src.const_pool, *(CONSTANT_ID*)(ip += OPCODE_SIZE));

			ObjectProperty *p;
			if ((p = scope_get_property(write_scope, key)) == NULL) {
				// Since the property create a new object property
				// with the given key and default options.
				ObjectProperty prop;
				object_property_init(&prop);

				prop.key = key;

				if ((prop.value = STACK_POP(thread.stack)).type 
						== VALUE_TYPE_STRING) {
					STRING_CLONE(prop.value.s_value, s);
					prop.value.s_value = s;
				}
			
				object_add_property(write_scope, prop);
			} else {
				// If the value is of type string, then
				// deallocate it before assigning a 
				// new value to the property.
				if (p->value.type == VALUE_TYPE_STRING) {
					free((char*)p->value.s_value.value);
				}

				// If c is a string then clone it, and push it instead of the 
				// original one.
				if ((p->value = STACK_POP(thread.stack)).type ==
						VALUE_TYPE_STRING) {
					Value v = p->value;
					STRING_CLONE(v.s_value, s);
					v.s_value = s;
					p->value = v;
				}

				// Resolve dependencies
				if (p->n_deps) {
					// The dependencies must be executed in the 
					// order they are present in the array.
					// However, to do so the frames must be 
					// pushed in reverse order onto the frame
					// stack.
					for (size_t i=p->n_deps; i > 0;) {

						Block dep = p->deps[--i];

						Frame f;
						f.block = dep;
						f.base_ptr = STACK_GET_FROM_TOP_PTR(thread.stack, 0);
						f.read_scope = dep.read_scope;
						f.write_scope = dep.write_scope;
						f.ip = dep.start_ptr;

						FRAME_STACK_PUSH(thread.fstack, f);
					}

					curr_frame->ip = ip + sizeof(CONSTANT_ID);

					JUMP_TO_STAGE_FETCH_CURRENT_FRAME();
				}
			}

			ip += sizeof(CONSTANT_ID);

		} END_HANDLER()

		START_HANDLER(SET_SCOPE_STACK) {
			Value key, value;
			key = STACK_POP(thread.stack);
			value = STACK_POP(thread.stack);

			// TODO: Check if the property is writable

			// If the value is a string then clone it,
			if (value.type == VALUE_TYPE_STRING) {
				STRING_CLONE(value.s_value, s);
				value.s_value = s;
			}

			// Write the value
			object_set_property_value(write_scope, key, value);

			ip += OPCODE_SIZE;

		} END_HANDLER()

		START_HANDLER(GET_OBJECT) {
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
				// If the property does not exist in the object
				// then look in the Object class.
				p = object_get_property(&OBJECT_CLASS, key);
 
				if (!p) {
					printf("fatal error: property '");
					value_log(key);
					printf("' not found in object ");
					value_log(obj);
					printf("\n");
					return;
				}
			} else {
				// Check if the property has a getter
				if (p->getter) {
					// If the property has a getter then call the
					// getter function with,
					// 1. me set to the object
					// 2. only one argument which is the value
					//    of the property.
					PAYLOADS.FUNC_CALL.func = p->getter;
					PAYLOADS.FUNC_CALL.is_async = FALSE;
					PAYLOADS.FUNC_CALL.is_me = TRUE;
					PAYLOADS.FUNC_CALL.me = obj;
					PAYLOADS.FUNC_CALL.n_params = 1;
					PAYLOADS.FUNC_CALL.params = &p->value;
					PAYLOADS.FUNC_CALL.rip = ip + OPCODE_SIZE;
					SUBMODULE_CALL(FUNC_CALL);

				} else {
					// If the property does not have a getter 
					// then just push the value onto the stack.
					STACK_PUSH(thread.stack, p->value);
				}
			}


			ip += OPCODE_SIZE;

		} END_HANDLER() 

		START_HANDLER(SET_OBJECT) {

			Value key = STACK_POP(thread.stack);
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

			//object_set_property_value(o.o_ptr, k, v);
			ObjectProperty *p;
			if ((p = object_find_property(o.o_ptr, key)) == NULL) {
				// Since the property create a new object property
				// with the given key and default options.
				ObjectProperty prop;
				object_property_init(&prop);

				prop.key = key;

				if ((prop.value = v).type 
						== VALUE_TYPE_STRING) {
					STRING_CLONE(prop.value.s_value, s);
					prop.value.s_value = s;
				}
			
				object_add_property(o.o_ptr, prop);
			} else {
				// If the value is of type string, then
				// deallocate it before assigning a 
				// new value to the property.
				if (p->value.type == VALUE_TYPE_STRING) {
					free((char*)p->value.s_value.value);
				}

				// If c is a string then clone it, and push it instead of the 
				// original one.
				if (v.type == VALUE_TYPE_STRING) {
					STRING_CLONE(v.s_value, s);
					p->value = v;
				}


				// Resolve dependencies
				if (p->n_deps) {
					// The dependencies must be executed in the 
					// order they are present in the array.
					// However, to do so the frames must be 
					// pushed in reverse order onto the frame
					// stack.

					for (size_t i=p->n_deps; i > 0;) {

						Block dep = p->deps[--i];

						Frame f;
						f.block = dep;
						f.base_ptr = STACK_GET_FROM_TOP_PTR(thread.stack, 0);
						f.read_scope = dep.read_scope;
						f.write_scope = dep.write_scope;
						f.ip = dep.start_ptr;

						FRAME_STACK_PUSH(thread.fstack, f);
					}

					curr_frame->ip = ip + OPCODE_SIZE;

					JUMP_TO_STAGE_FETCH_CURRENT_FRAME();
				}
			}
			

			ip += OPCODE_SIZE;

		} END_HANDLER() 

		START_HANDLER(BLOCK_NOEXEC_START) {
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

		} END_HANDLER()

		START_HANDLER(BLOCK_IMPLICIT_START) {

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

		} END_HANDLER()

		START_HANDLER(OBJECT_CREATE) { } END_HANDLER() 

		START_HANDLER(METHOD_CREATE) {
			Value v_body = STACK_POP(thread.stack);
			Value v_proto= STACK_POP(thread.stack);

			Object *func = class_instantiate(&METHOD_CLASS);
			method_class_new(NULL, value_object(func), NULL, value_int(0));

			if (v_proto.type != VALUE_TYPE_OBJECT) {
				// error.
			}

			if (v_body.type != VALUE_TYPE_BLOCK) {
				// error.
			}

			func_create(func, v_proto.o_ptr, v_body.bl_value, read_scope);

			STACK_PUSH(thread.stack, value_object(func));

			ip += OPCODE_SIZE;

		} END_HANDLER() 

		START_HANDLER(RETURN) {
			ip = ep;
		} END_HANDLER() 

		START_HANDLER(RETURN_NULL) {
			STACK_PUSH(thread.stack, NULL_VALUE);
			ip = ep;

		} END_HANDLER()

		START_HANDLER(JUMP_IF) {
			ip += OPCODE_SIZE + sizeof(size_t) + 
				(STACK_POP(thread.stack).b_value ? 
					*(const size_t*)(ip + OPCODE_SIZE) : 0);

		} END_HANDLER()

		START_HANDLER(JUMP_NIF) {
			size_t offset = 0;
			if (!(STACK_POP(thread.stack).b_value)) {
				offset = *(size_t*)(ip + OPCODE_SIZE);
			} 
			
			ip += OPCODE_SIZE + sizeof(size_t) + offset;

		} END_HANDLER()

		START_HANDLER(JUMP_IF_NOPOP) {
			ip += OPCODE_SIZE + sizeof(size_t) + 
				(STACK_GET_FROM_TOP(thread.stack, 1).b_value ? 
					*(const size_t*)(ip + OPCODE_SIZE) : 0);

		} END_HANDLER()

		START_HANDLER(JUMP_NIF_NOPOP) {
			size_t offset = 0;
			if (!(STACK_GET_FROM_TOP(thread.stack, 1).b_value)) {
				offset = *(size_t*)(ip + OPCODE_SIZE);
			} 
			
			ip += OPCODE_SIZE + sizeof(size_t) + offset;

		} END_HANDLER() 

		START_HANDLER(JUMP) {
			ip += OPCODE_SIZE + sizeof(size_t) + 
				*(size_t*)(ip + OPCODE_SIZE);

		} END_HANDLER()

		START_HANDLER(JUMP_BACK) {
			ip -= *(size_t*)(ip + OPCODE_SIZE);	
		} END_HANDLER() 



		START_HANDLER(INVOKE_CONSTANT_ME) {
			PAYLOADS.FUNC_CALL.is_async = FALSE;
			SUBMODULE_CALL(FUNC_CALL_ME);
		}

		START_HANDLER(INVOKE_CONSTANT_ME_ASYNC) {
			PAYLOADS.FUNC_CALL.is_async = TRUE;
			SUBMODULE_CALL(FUNC_CALL_ME);
		}

		START_HANDLER(INVOKE_CONSTANT) {
			PAYLOADS.FUNC_CALL.is_async = FALSE;
			SUBMODULE_CALL(FUNC_CALL_NO_ME);
		}

		START_HANDLER(INVOKE_CONSTANT_ASYNC) {
			PAYLOADS.FUNC_CALL.is_async = TRUE;
			SUBMODULE_CALL(FUNC_CALL_NO_ME);
		} 



		SUBMODULE_START(FUNC_CALL_ME) {
			PAYLOADS.FUNC_CALL.is_me = TRUE;

			// Pop the property name
			Value fname = STACK_POP(thread.stack);

			// Pop the object
			Value obj = STACK_POP(thread.stack);
			ObjectProperty *m = NULL;

			switch (obj.type) {
			case VALUE_TYPE_OBJECT:
				if (obj.o_ptr) {
					m = object_find_member(obj.o_ptr, fname);
				}
				break;
			case VALUE_TYPE_STRING:
				m = object_find_member(&STRING_CLASS, fname);
				break;
			case VALUE_TYPE_INT:
				m = object_find_member(&INT_CLASS, fname);
				break;
			case VALUE_TYPE_FLOAT:
				m = object_find_member(&FLOAT_CLASS, fname);
				break;
			case VALUE_TYPE_BOOLEAN:
				m = object_find_member(&BOOL_CLASS, fname);
				break;
			default:
				value_log(obj);
				printf(" is not an object\n");
				return;
			}

			if(!m) {
				printf("no member method with the name '");
				value_log(fname);
				printf("' inside ");
				value_log(obj);
				printf("\n");
				return;
			} else if (m->value.type != VALUE_TYPE_OBJECT) {
				value_log(m->value); 
				printf("is not an object.\n");
				return;
			} 

			PAYLOADS.FUNC_CALL.me = obj;

			PAYLOADS.FUNC_CALL.func = m->value.o_ptr;

			PAYLOADS.FUNC_CALL.n_params = *(FzInt*)(ip += OPCODE_SIZE);

			PAYLOADS.FUNC_CALL.params = STACK_GET_FROM_TOP_PTR(thread.stack, 
				PAYLOADS.FUNC_CALL.n_params);

			PAYLOADS.FUNC_CALL.rip = (ip += sizeof(FzInt));

		} SUBMODULE_CALL(FUNC_CALL);

		SUBMODULE_START(FUNC_CALL_NO_ME) {
			PAYLOADS.FUNC_CALL.is_me = FALSE;

			// Pop the function.
			Value m = STACK_POP(thread.stack);

			if (m.type != VALUE_TYPE_OBJECT || !m.o_ptr) {
				printf("'");
				LOG_VALUE(m);
				printf("' is not a function.\n");
				return;
			}

			PAYLOADS.FUNC_CALL.func = m.o_ptr;

			PAYLOADS.FUNC_CALL.n_params = *(FzInt*)(ip += OPCODE_SIZE);

			PAYLOADS.FUNC_CALL.params = STACK_GET_FROM_TOP_PTR(thread.stack, 
				PAYLOADS.FUNC_CALL.n_params);

			PAYLOADS.FUNC_CALL.rip = (ip += sizeof(FzInt));

		} SUBMODULE_CALL(FUNC_CALL);



		SUBMODULE_START(FUNC_CALL) {

			// Store the function object in fc.
			Object *fc = PAYLOADS.FUNC_CALL.func;

			// Get the number of parameters.
			size_t n_args = PAYLOADS.FUNC_CALL.n_params;
			Value *args = PAYLOADS.FUNC_CALL.params;

			Value me = VALUE_NULL;
			if (PAYLOADS.FUNC_CALL.is_me) {
				me = PAYLOADS.FUNC_CALL.me;
			}

			// TODO: Check whether the object is of type callable.

			// Check if function is a native function.
			if (func_get_native(fc)) {
				Block func_block = func_get_block(fc);
				NativeFunc f = (NativeFunc)func_block.start_ptr;

				Value r = f(fc, me, args, n_args);

				// Erase the params and the function from the stack.
				STACK_SET_TOP(
					thread.stack,
					args);
				
				STACK_PUSH(thread.stack, r);
				
			} else {
				// For now only allow normal variables.
				Object *origin_scope = NULL;

				ObjectProperty *params_p;
				ObjectProperty *origin_p;

				params_p = object_get_property(fc, value_string(to_string("params")));
				origin_p = object_get_property(fc, value_string(to_string("scope")));
				
				if (origin_p && origin_p->value.type == VALUE_TYPE_OBJECT) {
					origin_scope = origin_p->value.o_ptr;
				}

				if (params_p && params_p->value.type == VALUE_TYPE_OBJECT) {

					// Create the new read and write.
					Object *scope = malloc(sizeof(Object));
					object_init(scope);
					scope_create(scope, origin_scope, NULL);

					ObjectProperty *params;
					size_t n_params;

					if (params_p->value.o_ptr) {
						params = params_p->value.o_ptr->props;
						n_params = params_p->value.o_ptr->n_props;
					} else {
						params = NULL;
						n_params = 0;
					}

					size_t n = n_params > n_args ? n_args : n_params;

					for (size_t k=0; k < n; ++k) {
						object_set_property_value(scope, params[k].key, args[k]);
					}

					// Set the value of 'me'
					object_set_property_value(scope, FUNC_ME_PROP_NAME, me);

					// Store the address of the instruction that
					// is to be executed after returning from the 
					// function.
					curr_frame->ip = PAYLOADS.FUNC_CALL.rip;

					// Setup the frame for the function.
					Frame f;
					f.block = func_get_block(fc);
					f.read_scope = scope;
					f.write_scope = scope;
					f.ip = f.block.start_ptr;

					if (PAYLOADS.FUNC_CALL.is_async) {
						// If the call is an asynchronous one then detach the 
						// function from the current thread.
						Thread t;
						thread_init(&t);

						// Push the stack frame.
						f.base_ptr = STACK_GET_FROM_TOP_PTR(t.stack, 0);
						FRAME_STACK_PUSH(t.fstack, f);

						thread_pool_add(&i->thread_pool, t);

					} else {

						// Erase the arguments along with the function
						// object itself from the stack.
						STACK_SET_TOP(thread.stack, args);

						// Set the base pointer for the stack. All values
						// below the base pointer in the stack, are not 
						// accessible to this block.
						f.base_ptr = thread.stack.top_ptr;

						// Push the frame onto the frame stack.
						FRAME_STACK_PUSH(thread.fstack, f);

						// Jump to the frame fetch stage.
						JUMP_TO_STAGE_FETCH_CURRENT_FRAME();
					}
				} else {
					// Function object must contain a params property.
					printf("fatal error: function object must contain 'params' property\n");
					return;
				}
			} 

		} END_HANDLER()

		START_HANDLER(STACK_DUP_TOP) {
			STACK_PUSH(thread.stack, STACK_GET_FROM_TOP(thread.stack, 1));
			ip += OPCODE_SIZE;
		
		} END_HANDLER() 

		START_HANDLER(CLASSIFY) {

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

		} END_HANDLER()

		START_HANDLER(INSTANTIATE) {
			Value clazz = STACK_POP(thread.stack);
			if (clazz.type != VALUE_TYPE_OBJECT) {
				printf("fatal error: class is not an object\n");
				return;
			}

			// Instantiate the class to create an object.
			Object *o = class_instantiate(clazz.o_ptr);
			STACK_PUSH(thread.stack, value_object(o));

			ip += OPCODE_SIZE + sizeof(FzInt);

		} END_HANDLER() 

		START_HANDLER(MODULE_LOAD) {
			Value path = STACK_POP(thread.stack);

			if (path.type != VALUE_TYPE_STRING) {
				printf("fatal error(use): ");
				value_log(path);
				printf(" is not a string\n");
				return;
			}

			// Conver the path from module style path to
			// os style path.
			int ok;
			string opath = mpath_to_opath(path.s_value, &ok);

			SOURCE_ID sid = src_manager_load(&i->src_manager, opath);
			if (!sid) {
				printf("fatal error: failed to load module '");
				value_log(path);
				printf("'\n");
				return;
			}

			// Destroy the opath.
			free((char*)opath.value);

			Source src = src_manager_get(&i->src_manager, sid);

			Frame f;
			f.block.type = BLOCK_TYPE_EXTERNAL;
			f.block.source = SRC_MANAGER_GET_PTR(i->src_manager, sid);
			f.block.start_ptr = src.bcode.buffer;
			f.block.end_ptr = src.bcode.buffer + src.bcode.length;

			f.base_ptr = STACK_GET_FROM_TOP_PTR(thread.stack, 0);
			f.ip = f.block.start_ptr;

			Object *s = malloc(sizeof(Object));
			object_init(s);
			scope_create(s, NULL, NULL);

			f.read_scope = s;
			f.write_scope = s;

			FRAME_STACK_PUSH(thread.fstack, f);

			curr_frame->ip = (ip += OPCODE_SIZE);

			JUMP_TO_STAGE_FETCH_CURRENT_FRAME();

		} END_HANDLER() 

		START_HANDLER(BLOCK_NOEXEC_IMPLICIT_START) {
			size_t offset = *(size_t*)(ip + OPCODE_SIZE);

			Block b;
			b.type = BLOCK_TYPE_NOEXEC_IMPLICT;
			b.start_ptr = ip + OPCODE_SIZE + sizeof(size_t);
			b.end_ptr = b.start_ptr + offset;
			b.source = curr_frame->block.source;
			b.read_scope = read_scope;
			b.write_scope = write_scope;

			STACK_PUSH(thread.stack, value_block(b));

			ip = b.end_ptr + OPCODE_SIZE;

		} END_HANDLER()

		START_HANDLER(SET_SCOPE_DEPENDANT_STACK) {

			// Pop the key.
			Value key = STACK_POP(thread.stack);

			// Pop the block.
			Value block = STACK_GET_FROM_TOP(thread.stack, 1);

			if (block.type != VALUE_TYPE_BLOCK) {
				printf("fatal error: dependant must be a block, instead recieved: ");
				value_log(block);
				printf("\n");
			}

			ObjectProperty *p = scope_get_property(read_scope, key);
			if (!p) {
				printf("fatal error: cannot add dependant to property '");
				value_log(key);
				printf("' that does not exist\n");
				return;
			}

			object_property_add_dependant(p, block.bl_value);

			ip += OPCODE_SIZE;
		} END_HANDLER()

		START_HANDLER(SET_OBJECT_DEPENDANT) {

			// Pop the key.
			Value key = STACK_POP(thread.stack);
			// Pop the object.
			Value obj = STACK_POP(thread.stack);
			// Pop the block.
			Value block = STACK_GET_FROM_TOP(thread.stack, 1);

			if (block.type != VALUE_TYPE_BLOCK) {
				printf("fatal error: dependant must be a block, instead recieved: ");
				value_log(block);
				printf("\n");
			}

			ObjectProperty *p = scope_get_property(obj.o_ptr, key);
			if (!p) {
				printf("fatal error: cannot add dependant to property '");
				value_log(key);
				printf("' that does not exist\n");
				return;
			}

			object_property_add_dependant(p, block.bl_value);

			ip += OPCODE_SIZE;

		} END_HANDLER()

		START_HANDLER(LISTIFY) {
			size_t count = *(FzInt*)(ip += OPCODE_SIZE);
			ip += sizeof(FzInt);

			Value *args = STACK_GET_FROM_TOP_PTR(thread.stack, count);

			Object *o = class_instantiate(&LIST_CLASS);
			STACK_SET_TOP(thread.stack, args);
			STACK_PUSH(thread.stack, list_class_method_new(
				NULL,
				value_object(o), args, 
				count
				));

		} END_HANDLER()

		START_HANDLER(ATTACH_OBJECT) {

			// Property name
			Value k = STACK_POP(thread.stack);

			// Object
			Value o = STACK_POP(thread.stack);

			// Payload
			Value payload = STACK_POP(thread.stack);

			if (o.type != VALUE_TYPE_OBJECT) {
				printf("attach.object requires an object\n");
				return;
			}

			if (!o.o_ptr) {
				printf("cannot attach to a property of null\n");
				return;
			}

			ObjectProperty *p = object_find_member(o.o_ptr, k);
			if (p) {
				object_property_attach_payload(p, payload.o_ptr);
			} else {
				printf("cannot attach to a non existant property\n");
				return;
			}

			ip += OPCODE_SIZE;
		} END_HANDLER()

		START_HANDLER(ATTACH_SCOPE) {

			// Property name
			Value k = STACK_POP(thread.stack);

			// Payload
			Value payload = STACK_POP(thread.stack);


			STACK_POP(thread.stack);
			ip += OPCODE_SIZE;
		} END_HANDLER()

		// In case the opcode is an invalid one.
		{
			printf("fatal error: unknown opcode (0x%d 0x%d)\n", ip[0], ip[1]);
			return;
		} 

	CYCLE_UPDATE()

		goto __start;

		CYCLE_STAGE_NEXT_THREAD()
			// Save the info of the current frame such
			// as the instruction pointer and other
			// variables that define the state of the
			// thread.
			curr_frame->ip = ip;

			// Update the current thread.
			thread_iter->thread = thread;

			// Change the thread to the next living thread
			// before proceeding to the next cycle.
			thread_iter = thread_iter->next;

			#ifdef DEBUG
				printf("\033[1;35mcycle change\033[0m\n");
			#endif

			JUMP_TO_STAGE_THREAD_SELECT();

	} CYCLE_END()



	return;
}

string mpath_to_opath(string m, int *ok) {
	// A module path must consist of 
	// alphabets, digits, _ and .
	string p = STRING_NEW(NULL, 0);

	// +3 for the '.fz' extension
	char *str = calloc(m.length, sizeof(char) + 3);

	for (size_t i=0; i < m.length; ++i) {
		char c = m.value[i];
		
		if (c == '.') {
			str[i] = '/';
		} else if (!is_digit(c) && !is_letter(c) && c != '_') {
			*ok = FALSE;
			free(str);
			return p;
		} else {
			str[i] = c;
		}

		if (i > 0 && str[i] == '/' && str[i-1] == '/') {
			*ok = FALSE;
			free(str);
			return p;
		}
	}

	if (str[m.length-1] == '/') {
		*ok = FALSE;
		free(str);
		return p;
	}

	memcpy(str + m.length, ".fz", 3);

	p.value = str;
	p.length = m.length + 3;
	*ok = TRUE;

	return p;
}