#pragma once

#include "common.h"
#include "str.h"
#include "runtime/block.h"
#include "runtime/value.h"

typedef enum {
	VALUE_TYPE_OBJECT,
	VALUE_TYPE_INT,
	VALUE_TYPE_FLOAT,
	VALUE_TYPE_BOOLEAN,
	VALUE_TYPE_STRING,
	VALUE_TYPE_BLOCK,
} VALUE_TYPE;

#define VALUE_TYPE_PREFIXES \
	X(OBJECT, object, Object*, o_ptr) \
	X(INT, int, FzInt, i_value) \
	X(FLOAT, float, FzFloat, f_value) \
	X(BOOLEAN, bool, FzBool, b_value) \
	X(STRING, string, FzString, s_value) \
	X(BLOCK, block, Block, bl_value)


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
		Block bl_value;
	};
};

enum {
	VALUE_COMPARISON_EQUAL,
	VALUE_COMPARISON_DIFFERENT_TYPE,
	VALUE_COMPARISON_DIFFERENT_VALUE,
};

extern int compare_values(Value, Value);
extern int value_equals(Value, Value);

extern void value_log(Value);

// Declaration for functions value_string, ... etc.
// Used to create values out of primitives easily.
#define X(a, b, c, d) extern Value value_##b(c);
	VALUE_TYPE_PREFIXES
#undef X

// Declaration for the value_cstring to directly
// create values of type without converting them
// to string using the to_string() function.
#define value_cstring(x) value_string(to_string(x))

// Static initializers for values of different types.
#define VALUE_STRING(x) {.type=VALUE_TYPE_STRING, .s_value=x}
#define VALUE_INT(x) {.type=VALUE_TYPE_INT, .i_value=x}
#define VALUE_FLOAT(x) {.type=VALUE_TYPE_FLOAT, .f_value=x}
#define VALUE_BOOLEAN(x) {.type=VALUE_TYPE_BOOLEAN, .b_value=x}
#define VALUE_BLOCK(x) {.type=VALUE_TYPE_BLOCK, .bl_value=x}
#define VALUE_OBJECT(x) {.type=VALUE_TYPE_OBJECT, .o_ptr=x}
#define VALUE_CSTRING(x, y) VALUE_STRING(STRING_NEW(x, y))


extern const Value VALUE_NULL;
extern const Value VALUE_FALSE;
extern const Value VALUE_TRUE;


struct fz_object_property {
	Value key;
	Value value;

	// Setter and getter for the property.
	Object *setter, *getter;

	// Dependants.
	Block *deps;

	// Number of dependants.
	size_t n_deps;

	// Hidden values cannot be accessed by members outside the object.
	int is_hidden;

	// Whether the property is mutable or not i.e
	// if the value of the property can be changed.
	int is_mutable;

	// Behavioural attributes are functions only.
	int is_behaviour;
};

extern void object_property_init(ObjectProperty*);

extern void object_property_add_dependant(ObjectProperty*, Block);

/**
 * Used to attach a payload to the property.
 * The payload must be of type Object.
 * The payload can contain the following properties,
 * 
 * 1. get: If present, then its value is attached as the getter
 *         of the property.
 * 2. set: If present, then its value is attached as the setter
 *         of the property.
 */
extern void object_property_attach_payload(ObjectProperty*, Object*);

// An object is a set of key, value pairs where the key serves
// as the unique id. There can be only one pair with a given key.
// Apart from that you can attach listeners to a certain key, which
// will then listen for changes related to that specific pair.
struct fz_object {

	// Pointer to the class that was used to instantiate the 
	// object. In case of non-instantiated objects, this field
	// is set to NULL.
	Object *blueprint;

	ObjectProperty *props; 
	// The number of properties of the current object.
	size_t n_props;

	// Used by C defined classes to store native objects
	// Example: The List class uses it to store the underlying
	// List struct.
	void *meta;

	// Used by C defined classes to specify the size of the meta
	// object, so that it can be created at once by setting the
	// size of the object to sizeof(Object) + meta_size. This
	// saves the headache of dynamically allocating the meta
	// object and deallocating it later at the end of the lifetime
	// of the object.
	size_t meta_size;

};

extern void object_init(Object*);

extern void object_add_property(Object*, ObjectProperty);

extern void object_add_properties(Object*, ObjectProperty*, size_t);

extern int object_search_nth_property(Object*, Value, int);

#define object_search_property(o, v) object_search_nth_property(o, v, 1)

// Returns a pointer to the object property with the given
// key, if found. It returns NULL otherwise.
extern ObjectProperty* object_get_property(Object *o, Value k);

extern void object_set_property (Object *o, ObjectProperty);

extern void object_set_property_value(Object *o, Value k, Value v);

extern void object_log(Object*);


/**
 * This function searches for a property in the object only and returns
 * a pointer to it. However, it does not look for a property in
 * its blueprint or base classes. 
 */
extern ObjectProperty* object_find_property(Object*, Value);

/**
 * This function searches for a class member in an object.
 * with the given name. It first searches in the current object.
 * If not found then it searches in the class blueprint.
 * Again if not found, it searches in the base class if any.
 * If not found in any base class, it is looked for in the 
 * object class.
 */
extern ObjectProperty* object_find_member(Object *o, Value k);

/**
 * Searches for a member with the given name and sets its
 * value. If the member is not presents, then it creates 
 * a new member with the given key and value.
 */
extern int object_set_member_value(Object *o, Value k, Value v);

extern int object_member_attach_payload(Object *o, Value k, Value p);


