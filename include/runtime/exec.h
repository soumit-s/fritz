#pragma once

#include "runtime/block.h"
#include "runtime/instance.h"


extern void instance_exec(Instance*);
extern void instance_exec_threads(Instance*);

#define DECLARE_PAYLOAD(x) PAYLOAD_##x x;
#define SUBMODULE_START(x) __SUBMODULE_##x: 
#define SUBMODULE_END()
#define SUBMODULE_CALL(x) goto __SUBMODULE_##x

typedef struct {
	Object *func;
	Value *params;
	FzInt n_params;
	int is_async;
	int is_me;
	Value me;
	const uint8_t* rip;
} PAYLOAD_FUNC_CALL;

typedef struct {
	DECLARE_PAYLOAD(FUNC_CALL)
} SUBMODULE_PAYLOADS;


#define CYCLE_START() __start:
#define CYCLE_END() goto __start; __end:
#define CYCLE_INIT() __init:
#define CYCLE_UPDATE() __update:

#define JUMP_TO_CYCLE_UPDATE() goto __update

#define CYCLE_STAGE_FETCH_CURRENT_FRAME() __current_frame_fetch:
#define JUMP_TO_STAGE_FETCH_CURRENT_FRAME() goto __current_frame_fetch

#define CYCLE_STAGE_CURRENT_FRAME_DEPS_INIT() __current_frame_deps_init:
#define JUMP_TO_STAGE_CURRENT_FRAME_DEPS_INIT() goto __current_frame_deps_init

#define CYCLE_STAGE_THREAD_SELECT() __current_thread_select:
#define JUMP_TO_STAGE_THREAD_SELECT() goto __current_thread_select

#define CYCLE_STAGE_NEXT_THREAD()  __next_thread:
#define JUMP_TO_STAGE_NEXT_THREAD() goto __next_thread


#define START_HANDLER(n) OPCODE_HANDLER_##n:
#define END_HANDLER() JUMP_TO_CYCLE_UPDATE();

#define GOTO_OPCODE_HANDLER(n) goto OPCODE_HANDLER_##n;

//#define CALCULATE_OPCODE_HANDLER_OFFSET(n)

#define BEGIN_MAP() void *OPCODE_HANDLER_MAP[0xff];

#define MAP(n) \
	OPCODE_HANDLER_MAP[OPCODE_##n[1]] = &&OPCODE_HANDLER_##n;

#define END_MAP()

#define CHOOSE_HANDLER(o) goto *OPCODE_HANDLER_MAP[o[1]]
