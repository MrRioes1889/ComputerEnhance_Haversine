#include "platform/platform.h"
#include "haversine_generator.h"
#include <stdio.h>

int main(int argc, char** argv)
{
	SHM_PlatformContext plat_context = {0};
	platform_context_init(&plat_context);
	platform_console_window_open();

	char json_path[256] = {0};
	sprintf_s(json_path, 256, "%s\\haversine_pairs.json", plat_context.executable_dir);
	generate_haversine_test_json(json_path, 10, 10000000000003);

	platform_console_window_close();
	return 0;
}
