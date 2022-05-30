#include "runtime/universal/int.h"

#include "runtime/class.h"
#include "runtime/universal/method.h"

Object INT_CLASS;
Value INT_CLASS_NAME = VALUE_CSTRING("Int", 3);

void int_class_init(Object *o) {
	object_init(o);
	class_create(o, NULL, INT_CLASS_NAME);

	ADD_METHOD(o, "__toStr", &int_class_method___toStr);
}

DEFINE_NATIVE_METHOD(int_class_method___toStr){
	if (me.type != VALUE_TYPE_INT) {
		return VALUE_NULL;
	}

	#define n me.i_value

	// Calaculate the length of the string
	int c = 1;
	for (int t = 10; n / t != 0 ; t *= 10, c++);
	
	// If the number is less than 0, then 
	// the '-' signed will take an extra 
	// character
	if (n < 0) {  
		c++;
	}

	string s;
	s.value = malloc(c * sizeof(char));
	s.length = c;

	if (n < 0) {
		((char*)s.value)[0] = '-';
	}

	// Strip off the - sign
	if (n < 0) {
		n = -n;
	}

	do {
		((char*)s.value)[--c] = (char)(n % 10) + '0';
		n /= 10;
	} while (n != 0);

	#undef n

	return value_string(s);
}