#include <stdio.h>

#include "fend/fritz.h"
#include "str.h"
#include "tests.h"

int main(int argc, const char **args) {
	//#ifdef DEBUG

	
	//tests_init();
	//test_tokenizer();
	//test_parser();
	//test_compiler();
	//tests_over();
	

	//#endif

	return fritz_cli_exec(args+1, argc-1);
}