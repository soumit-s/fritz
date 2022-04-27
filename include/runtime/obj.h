#pragma once

#include "common.h"
#include "str.h"

typedef enum {
	VALUE_TYPE_OBJECT,
	VALUE_TYPE_INT,
	VALUE_TYPE_FLOAT,
	VALUE_TYPE_BOOLEAN,
	VALUE_TYPE_STRING,
} VALUE_TYPE;

typedef struct fz_value Value;
typedef struct fz_object_property ObjectProperty;
typedef struct fz_object Object;

struct fz_value {
	VALUE_TYPE type;
	union {
		FzInt i_value;
		FzFloat f_value;
		FzBool b_value;
		FzString s_value;
		Object *o_ptr;
	};
};

enum {
	VALUE_COMPARISON_EQUAL,
	VALUE_COMPARISON_DIFFERENT_TYPE,
	VALUE_COMPARISON_DIFFERENT_VALUE,
};

extern int compare_values(Value, Value);


struct fz_object_property {
	Value key;
	Value value;

	// Whether the property is mutable or not i.e
	// if the value of the property can be changed.
	int is_mutable;

	// Behavioural attributes are functions only.
	int is_behaviour;
};

extern void object_property_init(ObjectProperty*);

// An object is a set of key, value pairs where the key serves
// as the unique id. There can be only one pair with a given key.
// Apart from that you can attach listeners to a certain key, which
// will then listen for changes related to that specific pair.
struct fz_object {

	ObjectProperty *props; 
	// The number of properties of the current object.
	size_t n_props;

};

extern void object_init(Object*);

extern void object_add_property(Object*, ObjectProperty);

extern int object_search_nth_property(Object*, Value, int);

#define object_search_property(o, v) object_search_nth_property(o, v, 1)

// Returns a pointer to the object property with the given
// key, if found. It returns NULL otherwise.
extern ObjectProperty* object_get_property(Object *o, Value k);


