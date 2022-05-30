#include "runtime/class.h"
#include "runtime/obj.h"
#include "runtime/universal/method.h"

Value SUPER_PROP_NAME = VALUE_CSTRING("super", 5);
Value CLASS_NAME_PROP_NAME = VALUE_CSTRING("name", 4);
Value ME_PROP_NAME = VALUE_CSTRING("me", 2);

Value CONSTRUCTOR_METHOD_NAME = VALUE_CSTRING("new", 3);

Object* class_create(Object *c, Object *s, Value n) {
	// A class consists of some mandatory properties such 
	// as super, me and hidden.
	// super: This property is used to stores the object which
	//        represents its base class. If the class does not have
	//        a base class then , this property is null.
	// name: The name of the class.

	if(c == NULL) { return NULL; }

	object_set_property_value(c, SUPER_PROP_NAME, value_object(s));

	object_set_property_value(c, CLASS_NAME_PROP_NAME, n);

	return c;
}

Object* class_instantiate(Object *c) {
	// An instantiated object created using the 'new' operator 
	// consists of some mandatory properties such 
	// as super, me and hidden.
	// super: This property is used to stores the object which
	//        represents its base class. If the class does not have
	//        a base class then , this property is null.
	// me: Similar to self, this in other languages, it points to the
	//     object itself.
	// hidden: It is used to store values that cannot be accessed by 
	//         any piece of code except by the class methods.

	Object *o = malloc(sizeof(Object) + c->meta_size);
	object_init(o);
	o->blueprint = c;

	if ( c->meta_size) {
		o->meta = o + 1;
	}

	// Copy the properties of the class to the object.
	for (size_t i=0; i < c->n_props; ++i) {
		ObjectProperty p = c->props[i];

		
		if (value_equals(p.key, CLASS_NAME_PROP_NAME) ||
			value_equals(p.key, SUPER_PROP_NAME)) {
			continue;
		} else if (p.value.type == VALUE_TYPE_OBJECT && 
			p.value.o_ptr && p.value.o_ptr->blueprint == &METHOD_CLASS) {
			continue;
		} /*else if (p.value.type == VALUE_TYPE_OBJECT && p.value.o_ptr != NULL) {
			Object *x = p.value.o_ptr;
			if (x->blueprint == &METHOD_CLASS) {
				// In case it is a function, clone the function 
				// and set its 'me' value to the current object.
				Object *clone = calloc(1, sizeof(Object) + x->meta_size);
				*clone = *x;

				Method *meta = clone->meta;
				meta->me = o;

				p.value.o_ptr = clone;
			}
		}*/

		object_add_property(o, p);
	}


	// After adding the above properties, the 'new' function is called.
	// If a function named new is not present in the class, then this 
	// step is skipped.

	return o;
}