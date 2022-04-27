#include <stdio.h>

#include "tests.h"

int main() {
	tests_init();
	test_tokenizer();
	test_parser();
	test_compiler();
	tests_over();
	return 0;
}