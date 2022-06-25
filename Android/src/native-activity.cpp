#include "android_native_app_glue.h"
#include "logging.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <android/sensor.h>
#include <string>



#include <dlfcn.h>
#include "Plugins/PluginCommon.h"
#include <vector>

#define DYNAMIC_LIBRARY_EXTENSION ".so"
#define LOAD(file) dlopen(file, RTLD_NOW)
#define SYM(handle, procName) dlsym((void*)handle, procName)
#define FREE(handle) dlclose((void*)handle)

typedef PluginClass* (*GetPlugFunc)();
PluginClass* TryLoadPlugin(const char* pluginName)
{
    LOG("TRYING TO LOAD: %s\n", pluginName);
    void* handle = LOAD(pluginName);
    if(!handle) return nullptr;

    GetPlugFunc func = (GetPlugFunc)SYM(handle, "GetPlugin");
    if(!func){
        FREE(handle);
        return nullptr;
    }
    PluginClass* resultPlugin = func();
    PLUGIN_INFO info = resultPlugin->GetPluginInfos();
    LOG("LOADED PLUGIN WITH INFO: %s", info.ID);
    return resultPlugin;
}
PluginClass* loadedPlugin = nullptr;






struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};
JNIEnv* env = nullptr;
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;
    bool initialized = false;
};

static int engine_init_display(struct engine* engine) {
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    EGLint w, h, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);


    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);


    GLint attribList[3]{
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE,
    };
    context = eglCreateContext(display, config, NULL, attribList);

    if(context == nullptr){
        LOG("CONTEXT NOT SUPPORTED\n");
    }
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOG("Unable to eglMakeCurrent\n");
        return -1;
    }

    if(!eglQuerySurface(display, surface, EGL_WIDTH, &w))
        w = 0;
    if(!eglQuerySurface(display, surface, EGL_HEIGHT, &h))
        h = 0;



    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;
    engine->state.angle = 0;

    if(loadedPlugin)
    {
        LOG("WIDTH/HEIGHT: %d, %d\n", w, h);
        loadedPlugin->sizeX = w; loadedPlugin->sizeY = h;
        loadedPlugin->framebufferX = w; loadedPlugin->framebufferY = h;
    }

    return 0;
}



static void engine_draw_frame(struct engine* engine) {
    if (engine->display == NULL) {
        return;
    }

    if(loadedPlugin && engine->initialized) {
        glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        loadedPlugin->Render(nullptr);
    }
    eglSwapBuffers(engine->display, engine->surface);
}
static void engine_term_display(struct engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->state.x = AMotionEvent_getX(event, 0);
        engine->state.y = AMotionEvent_getY(event, 0);
        return 1;
    }
    return 0;
}
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
                if(loadedPlugin) {
                    loadedPlugin->Init(nullptr);
                    engine->initialized = true;
                }
            }
            break;
        case APP_CMD_TERM_WINDOW:
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                                               engine->accelerometerSensor);
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                                               engine->accelerometerSensor, (1000L / 60) * 1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                                                engine->accelerometerSensor);
            }
            engine->animating = 0;
            engine_draw_frame(engine);
            break;
        case APP_CMD_DESTROY:
            LOG("DESTROYING\n");
            break;
    }
}

bool GetStringParam(android_app* state, char paramBuffer[100])
{
    if(!env) {
        state->activity->vm->AttachCurrentThread(&env, 0);
    }
    jobject clazz = state->activity->clazz;
    jclass acl = env->GetObjectClass(clazz);
    jmethodID giid = env->GetMethodID(acl, "getIntent", "()Landroid/content/Intent;");
    jobject intent = env->CallObjectMethod(clazz, giid);
    jclass icl = env->GetObjectClass(intent);
    jmethodID gseid = env->GetMethodID(icl, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring jsParam = (jstring)env->CallObjectMethod(intent, gseid, env->NewStringUTF("PLUGIN"));
    if(jsParam != nullptr) {
        const char *param = env->GetStringUTFChars(jsParam, 0);

        if (param) {
            memcpy(paramBuffer, param, std::min(strlen(param) + 1, 100u));
            paramBuffer[99] = '\00'; // null terminator
        }
        env->ReleaseStringUTFChars(jsParam, param);
    }

    return jsParam;
}


bool LaunchJavaActivity(android_app* state)
{
    jobject curActivity = state->activity->clazz;
    jclass classNativeActivity = env->FindClass("android/app/NativeActivity");
    jclass contextClass = env->FindClass("android/content/Context");
    if(contextClass == nullptr) return false;
    jmethodID startActivityMethodID = env->GetMethodID(contextClass, "startActivity", "(Landroid/content/Intent;)V");
    if(startActivityMethodID == nullptr) return false;
    jclass intentClass = env->FindClass("android/content/Intent");
    if(intentClass == nullptr) return false;
    jmethodID intentConstructorMethodID = env->GetMethodID(intentClass, "<init>", "()V");
    if(intentConstructorMethodID == nullptr) return false;
    jmethodID intentSetActionMethodId = env->GetMethodID(intentClass, "setAction", "(Ljava/lang/String;)Landroid/content/Intent;");
    if(intentSetActionMethodId == nullptr) return false;

    jmethodID getClassLoader = env->GetMethodID(classNativeActivity, "getClassLoader", "()Ljava/lang/ClassLoader;");
    if(getClassLoader == nullptr) return false;

    jclass classLoader = env->FindClass("java/lang/ClassLoader");
    if(classLoader == nullptr) return false;
    jobject cls = env->CallObjectMethod(curActivity, getClassLoader);
    if(cls == nullptr) return false;
    jmethodID findClass = env->GetMethodID(classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if(findClass == nullptr) return false;


    jstring intentString = env->NewStringUTF("com.MiniGames.LobbyActivity");

    jobject intentObject = env->NewObject(intentClass, intentConstructorMethodID);
    if(intentObject == nullptr) return false;

    env->CallObjectMethod(intentObject, intentSetActionMethodId, intentString);

    env->CallVoidMethod(curActivity, startActivityMethodID, intentObject);

    state->activity->vm->DetachCurrentThread();

    return true;
}



void android_main(struct android_app* state)
{
    env = nullptr;
    loadedPlugin = nullptr;
    struct engine engine;
    char buffer[100];
    engine.initialized = false;

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    engine.sensorManager = ASensorManager_getInstance();
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
                                                                 ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
                                                              state->looper, LOOPER_ID_USER, NULL, NULL);

    if (state->savedState != NULL) {
        engine.state = *(struct saved_state*)state->savedState;
    }
    engine.animating = 1;

    if(GetStringParam(state, buffer))
    {
        loadedPlugin = TryLoadPlugin(buffer);
        if(!loadedPlugin) LaunchJavaActivity(state);
    }
    else
    {
        LaunchJavaActivity(state);
    }



    while(1)
    {
        int ident;
        int events;
        struct android_poll_source* source;
        while ((ident = ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0) {
            if (source != NULL) {
                source->process(state, source);
            }
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                                                       &event, 1) > 0) {
                        // LOG("accelerometer: x=%f y=%f z=%f\n", event.acceleration.x, event.acceleration.y, event.acceleration.z);
                    }
                }
            }
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }
        //engine_draw_frame(&engine);

    }

}