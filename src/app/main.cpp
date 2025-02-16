#include "application.h"

#include <GLFW/glfw3.h>
#include <webgpu-utils.h>
#include <webgpu/webgpu.h>

#include <cassert>
#include <cstring>

int main() {
	Application app;

	if (!app.initialize()) {
		return -1;
	}

	while (app.is_running()) {
		app.main_loop();
	}

    std::cout << "Terminating application" << std::endl;

	app.terminate();

	return 0;
}
