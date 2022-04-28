#pragma once

#include "common.h"

typedef struct fz_string string;

struct fz_string {
	const char* value;
	size_t length;
};

extern string string_new(const char*, size_t);

extern int string_eq(string, string);
extern int string_eqc(string, const char*);

extern string to_string(const char*);

extern long string_to_long(string);
extern long string_hex_to_long(string);
extern long string_octal_to_long(string);
extern long string_hex_to_long(string);
extern long string_bin_to_long(string);
extern double string_to_double(string);

extern string string_to_string(string);
