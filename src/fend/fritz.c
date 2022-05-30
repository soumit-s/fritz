#include "fend/fritz.h"
#include <string.h>
#include "common.h"
#include "runtime/instance.h"
#include "runtime/exec.h"
#include "str.h"
#include <stdio.h>

int fritz_cli_exec(const char **args, int argc) {

	const char *prog = NULL;

	size_t n_lpaths = 0;
	int i_lpath_start = -1;

	for (int i=0; i < argc; ++i) {
		const char *arg = args[i];

		if (strcmp(arg, "-h") == 0) {
			// Display help
		} else if (strcmp(arg, "-v") == 0) {
			// Print the version
			printf("%s (%s)\n", VERSION, PHASE);
		} else if (strncmp("-l", arg, 2) == 0) {
			n_lpaths ++;
			i_lpath_start = i_lpath_start == -1 ? i : i_lpath_start;
		} else {
			prog = arg;
		}
	}

	if (prog != NULL) {
		// Create the path to the program
		string path = STRING_NEW(prog, strlen(prog));

		int n_implicit_paths = 1;

		// Extract the library paths.
		string lpaths[n_lpaths + n_implicit_paths];
		lpaths[0] = to_string("/home/soumit/Projects/fritz/uni");

		for (int i = i_lpath_start, c = n_implicit_paths; i < argc; ++i) {
			const char *arg = args[i];
			if (strncmp("-l", arg, 2) == 0) {
				lpaths[c].value = arg + 2;
				if ((lpaths[c++].length = strlen(arg) - 2) == 0) {
					printf("\033[1;30minvalid argument \033[1;32m%s\033[1;30m," 
						"requires a path after -l \n\033[0", arg);
					return -1;
				}
			}
		}

		// Create an instance and execute the program.
		Instance instance;

		Instance *i = &instance;
		instance_init(i);

		// Set the library paths.
		instance.src_manager.lpaths = lpaths;
		instance.src_manager.n_lpaths = n_lpaths + n_implicit_paths;

		// Load the main file.
		src_manager_load(&instance.src_manager, path);

		// Start executing.
		instance_exec(i);

		instance_destroy(i);
	}

	return 0;
}