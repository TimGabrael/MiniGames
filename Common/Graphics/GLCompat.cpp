#include "GLCompat.h"
#include "logging.h"
#define LOG_DRAW_CALLS false



static GLuint currentlyBoundShaderProgram = 0;
static GLuint defaultFramebuffer = 0;
static GLuint mainFramebuffer = 0;
#if LOG_DRAW_CALLS
static int numDrawCallsMadeThisFrame = 0;
#endif

void EndFrameAndResetData()
{
#if LOG_DRAW_CALLS
	LOG("NUM_DRAW_CALLS: %d\n", numDrawCallsMadeThisFrame);
	numDrawCallsMadeThisFrame = 0;
#endif
	currentlyBoundShaderProgram = 0;
}

void glUseProgramWrapper(GLuint program)
{
	if (currentlyBoundShaderProgram != program)
	{
		glUseProgram(program);
		currentlyBoundShaderProgram = program;
	}
}
void glDrawElementsWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
#if LOG_DRAW_CALLS
	numDrawCallsMadeThisFrame = numDrawCallsMadeThisFrame + 1;
#endif
	glDrawElements(mode, count, type, indices);
}
void glDrawArraysWrapper(GLenum mode, GLint first, GLsizei count)
{
#if LOG_DRAW_CALLS
	numDrawCallsMadeThisFrame = numDrawCallsMadeThisFrame + 1;
#endif
	glDrawArrays(mode, first, count);
}
void glDrawElementsInstancedWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount)
{
#if LOG_DRAW_CALLS
	numDrawCallsMadeThisFrame = numDrawCallsMadeThisFrame + 1;
#endif
	glDrawElementsInstanced(mode, count, type, indices, instancecount);
}
void glDrawArraysInstancedWrapper(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
#if LOG_DRAW_CALLS
	numDrawCallsMadeThisFrame = numDrawCallsMadeThisFrame + 1;
#endif
	glDrawArraysInstanced(mode, first, count, instancecount);
}
void glEnableClipDistance(int clipIndex)
{
#ifdef ANDROID
	glEnable(GL_CLIP_DISTANCE0_EXT + clipIndex);
#else
	glEnable(GL_CLIP_DISTANCE0 + clipIndex);
#endif
}
void glDisableClipDistance(int clipIndex)
{
#ifdef ANDROID
	glDisable(GL_CLIP_DISTANCE0_EXT + clipIndex);
#else
	glDisable(GL_CLIP_DISTANCE0 + clipIndex);
#endif
}


void SetDefaultFramebuffer(GLuint fb)
{
	defaultFramebuffer = fb;
}
GLuint GetDefaultFramebuffer()
{
	return defaultFramebuffer;
}

void SetMainFramebuffer(GLuint fb)
{
	mainFramebuffer = fb;
}
GLuint GetMainFramebuffer()
{
	return mainFramebuffer;
}