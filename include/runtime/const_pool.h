#pragma once

#include "common.h"
#include "str.h"
#include "runtime/value.h"

typedef struct fz_constant Constant;
typedef struct fz_constant_pool_creator_list ConstantPoolCreatorList;
typedef struct fz_constant_pool_creator ConstantPoolCreator;

typedef struct fz_constant_pool_entry ConstantPoolEntry;
typedef struct fz_constant_pool ConstantPool;

typedef enum {
	CONSTANT_TYPE_FLOAT,
	CONSTANT_TYPE_INT,
	CONSTANT_TYPE_STRING,
	CONSTANT_TYPE_BOOLEAN,
} CONSTANT_TYPE;

struct fz_constant {
	CONSTANT_TYPE type;
	FzInt i_value; // Integer value
	FzBool b_value; // Boolean value
	FzFloat f_value; // Float value
	string s_value; // String value
};

struct fz_constant_pool_creator_list {
	size_t n_consts;
	size_t c_consts;
	Constant *consts;
};

struct fz_constant_pool_creator {
	ConstantPoolCreatorList *lists;
	size_t n_lists;
	size_t c_lists;
};

typedef unsigned long CONSTANT_ID;

extern void constant_pool_creator_init(ConstantPoolCreator*);
extern void constant_pool_creator_destroy(ConstantPoolCreator*);
extern CONSTANT_ID constant_pool_creator_add_constant(ConstantPoolCreator*, Constant);
extern int constant_pool_creator_search(ConstantPoolCreator*, Constant, CONSTANT_ID*);
extern CONSTANT_ID constant_pool_creator_append_constant(ConstantPoolCreator*, Constant);


struct fz_constant_pool_entry {
	CONSTANT_TYPE type;
	void *value_ptr;
};

// The Constant Pool consists of a linked list of
// constants and an contigious hash map that maps
// the constants to a numeric id. 
struct fz_constant_pool {
	Value *consts;
	size_t n_consts;
};

extern void constant_pool_init(ConstantPool*);
extern void constant_pool_destroy(ConstantPool*);
extern void constant_pool_create(ConstantPool*, ConstantPoolCreator*);

#define constant_pool_get_type(x, y) x.consts[x].type
#define constant_pool_get_value(x, y) x.consts[y]
#define constant_pool_get_int_value(x, y) x.consts[y].i_value
#define constant_pool_get_float_value(x, y) x.consts[y].f_value
#define constant_pool_get_string_value(x, y) x.consts[y].s_value
#define constant_pool_get_bool_value(x, y) x.consts[y].b_value

