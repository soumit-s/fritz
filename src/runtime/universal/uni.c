#include "runtime/universal/uni.h"
#include "runtime/obj.h"
#include "runtime/universal/helper.h"

#include <stdlib.h>
#include <sys/utsname.h>

DEFINE_NATIVE_METHOD(uni_exec_cmd) {
	if (n_args && args[0].type == VALUE_TYPE_STRING) {
		char *cmd = to_cstring(args[0].s_value);
		int r = system(cmd);
		free(cmd);
		return value_int(r);
	}
	return VALUE_NULL;
}

DEFINE_NATIVE_METHOD(uni_get_platform_info) {
	if (!n_args) {
		return VALUE_NULL;
	}

	if (args[0].type != VALUE_TYPE_OBJECT || !args[0].o_ptr) {
		return VALUE_NULL;
	}

	Object *o = args[0].o_ptr;

	struct utsname info;

	if (uname(&info) == -1) {
		return VALUE_NULL;
	}

	STRING_CLONE(to_string(info.sysname), sysname);
	STRING_CLONE(to_string(info.version), version);
	STRING_CLONE(to_string(info.release), release);
	STRING_CLONE(to_string(info.nodename), nodename);
	STRING_CLONE(to_string(info.machine), machine);

	object_set_member_value(o, value_cstring("system"), value_string(sysname));
	object_set_member_value(o, value_cstring("release"), value_string(release));
	object_set_member_value(o, value_cstring("version"), value_string(version));
	object_set_member_value(o, value_cstring("nodeName"), value_string(nodename));
	object_set_member_value(o, value_cstring("machine"), value_string(machine));

	return value_object(o);
}

DEFINE_NATIVE_METHOD(uni_file_class_init) {
	return VALUE_NULL;
}

DEFINE_NATIVE_METHOD(uni_file_class_method_open) {
	return VALUE_NULL;
}

DEFINE_NATIVE_METHOD(uni_file_class_method_close) {
	return VALUE_NULL;
}