#pragma once


#if defined(ANDROID) or defined(EMSCRIPTEN)
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3platform.h>
#else
#include "glad/glad.h"
#endif

void EndFrameAndResetData();

void glUseProgramWrapper(GLuint program);
void glDrawElementsWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices);
void glDrawArraysWrapper(GLenum mode, GLint first, GLsizei count);
void glDrawElementsInstancedWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount);
void glDrawArraysInstancedWrapper(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);

void glEnableClipDistance(int clipIndex);
void glDisableClipDistance(int clipIndex);

// THESE ARE NEEDED, AS QT/ANDROID_NATIVE_GLUE MAY FORCE A DIFFRENT DEFAULT FRAMEBUFFER
// IN THE CASE OF QT IT SHOWED THAT THE DEFAULT FB IS 1 USUALLY
// BUT THAT MAY ( MAY ) DIFFER ON EACH STARTUP WHO KNOWS WHAT QT IS UP TO :(
void SetDefaultFramebuffer(GLuint fb);
GLuint GetDefaultFramebuffer();

// THE FRAMEBUFFER ALL OBJECTS ARE SUPPOSED TO BE DRAWN TO, may differ from DefaultFramebuffer for example in Compositing/Bloom
void SetMainFramebuffer(GLuint fb);
GLuint GetMainFramebuffer();