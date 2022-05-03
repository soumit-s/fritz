#include <stdio.h>

#include "tests.h"

int main(int argc, const char **args) {
	//#ifdef DEBUG
	
	tests_init();
	test_tokenizer();
	test_parser();
	test_compiler();
	tests_over();

	//#else

	//#endif
	return 0;
}