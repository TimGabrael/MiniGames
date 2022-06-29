#include "emscripten.h"
#include "emscripten/html5.h"
#include "GLFW/glfw3.h"
#include "Plugins/PluginCommon.h"
#include "logging.h"

#include <stdio.h>
#include <dlfcn.h>


GLFWwindow* window = nullptr;

PluginClass* loadedPlugin = nullptr;

extern "C" PluginClass* GetPlugin();




void mainLoop() {

    loadedPlugin->Render(nullptr);

    glfwSwapBuffers(window);
    glfwPollEvents();
}







int main()
{  
    glfwInit();
    loadedPlugin = GetPlugin();
    LOG("loadedPlugin: %s\n", loadedPlugin->GetPluginInfos().ID);
    
    int sizeX = 640;
    int sizeY = 480;
    window = glfwCreateWindow(sizeX, sizeY, "Test Window", 0, 0);
    glfwMakeContextCurrent(window);


    loadedPlugin->Init(nullptr, PLATFORM_ID::PLATFORM_ID_EMSCRIPTEN);
    loadedPlugin->sizeX = sizeX;
    loadedPlugin->sizeY = sizeY;
    loadedPlugin->framebufferX = sizeX;
    loadedPlugin->framebufferY = sizeY;
    loadedPlugin->Resize(nullptr);


    

    emscripten_set_main_loop(&mainLoop, 0, 1);
   
}