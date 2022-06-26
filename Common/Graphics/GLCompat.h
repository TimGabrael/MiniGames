#pragma once


#if defined(ANDROID) or defined(EMSCRIPTEN)
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif

