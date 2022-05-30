#pragma once

#include "common.h"

typedef struct fz_string string;

struct fz_string {
	const char* value;
	size_t length;
};

// static initializer for string struct
#define STRING_NEW(v, l) {.value = v, .length = l}

extern string string_new(const char*, size_t);

extern int string_eq(string, string);
extern int string_eqc(string, const char*);

extern string to_string(const char*);
extern char* to_cstring(string s);

extern long string_to_long(string);
extern long string_hex_to_long(string);
extern long string_octal_to_long(string);
extern long string_hex_to_long(string);
extern long string_bin_to_long(string);
extern double string_to_double(string);

extern string string_to_string(string);

#define STRING_PRINT(s) for(int i=0; i < (s).length; ++i) printf("%c", (s).value[i]);

#define STRING_CLONE(s, c) \
	string c = STRING_NEW(malloc(s.length * sizeof(char)), s.length); \
	for (size_t n=s.length; n-- > 0;) ((char*)c.value)[n] = s.value[n];
		