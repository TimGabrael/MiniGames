#include "Simple3DRendering.h"


static const char* uiVertexShader = "#version 300 es\n\
\n\
layout(location = 0) in vec3 pos;\n\
layout(location = 1) in vec2 texPos;\n\
layout(location = 2) in vec4 color;\n\
uniform UBO\n\
{\n\
	mat4 projection;\n\
	mat4 model;\n\
	mat4 view;\n\
	vec3 camPos;\n\
} ubo;\n\
out vec2 tPos;\
out vec4 col;\
void main(){\
	gl_Position = ubo.model * ubo.projection * ubo.view * vec4(pos, 1.0f);\
	tPos = texPos;\
	col = color;\
}\
";

static const char* uiFragmentShader = "#version 300 es\n\
precision highp float;\n\
\n\
in vec2 tPos;\
in vec4 col;\
uniform sampler2D tex;\
out vec4 outCol;\
void main(){\
	outCol = texture(tex, tPos).rgba * col;\
}\
";



void InitializeSimple3DPipeline()
{

}