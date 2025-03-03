#include <webgpu/webgpu.h>
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <cassert>
#include <cstring>

#include "Application.hpp"

int main() {
    Application app;   

    if (!app.Initialize()) {
        return -1;
    }
    
    #ifdef __EMSCRIPTEN__
	auto callback = [](void *arg) {
		Application* pApp = reinterpret_cast<Application*>(arg);
		pApp->MainLoop(); // 4. We can use the application object
	};
	emscripten_set_main_loop_arg(callback, &app, 0, true);
#else // __EMSCRIPTEN__
	while (app.isRunning()) {
		app.MainLoop();
	}
#endif // __EMSCRIPTEN__

    app.Terminate();

    return 0;
}
