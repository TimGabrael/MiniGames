#include "emscripten.h"
#include "emscripten/html5.h"
#include "GLFW/glfw3.h"


#include <stdio.h>
#include <dlfcn.h>


GLFWwindow* window = nullptr;



void TestPluginLoading()
{
    void* module = dlopen("Uno.wasm", RTLD_NOW);
    printf("Module: %p\n", module);
    module = dlopen("Uno.js", RTLD_NOW);
    printf("Module: %p\n", module);
}

void mainLoop() {
    // Clear the screen with a color
    glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Swap the buffers of the window
    glfwSwapBuffers(window);

    // Poll for the events
    glfwPollEvents();
}


int main()
{  
    printf("MAIN WIRD AUFGERUFEN\n");
    TestPluginLoading();

    glfwInit();
    
    window = glfwCreateWindow(640, 480, "Test Window", 0, 0);
    glfwMakeContextCurrent(window);



    emscripten_set_main_loop(&mainLoop, 0, 1);
   
}