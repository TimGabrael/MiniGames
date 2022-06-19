#include "Uno.h"
#include <string>
#include "Graphics/Helper.h"
#include <iostream>

PLUGIN_EXPORT_DEFINITION(UnoPlugin, "a3fV-6giK-10Eb-2rdT");

PLUGIN_INFO UnoPlugin::GetPluginInfos()
{
	PLUGIN_INFO plugInfo;
	memcpy(plugInfo.ID, _Plugin_Export_ID_Value, strnlen_s(_Plugin_Export_ID_Value, 19));
	plugInfo.previewResource = nullptr;
	return plugInfo;

}



struct UnoGlobals
{
	GLuint shaderProgram;
	GLuint vertexBuffer;
	GLuint indexBuffer;
	GLuint vao;
}g_objs;

// Test Triangle
static const GLfloat g_vertex_buffer_data[] = {
	-0.5f, -0.5f, 0.0f,
	 0.5f, -0.5f, 0.0f,
	 0.0f,  0.5f, 0.0f
};
static const GLuint g_index_buffer_data[] = {
	0, 1, 2,
};



const char* vertexShaderSrc = "#version 330 core\n\
layout(location = 0) in vec3 aPos;\
\
void main()\
{\
	gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\
}";
const char* fragmentShaderSrc = "#version 330 core\n\
out vec4 FragColor;\
\
void main()\
{\
	FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\
}\
";


void UnoPlugin::Init(void* backendData)
{
	initialized = true;
	InitializeOpenGL();

	
	g_objs.shaderProgram = CreateProgram(vertexShaderSrc, fragmentShaderSrc);

	glGenVertexArrays(1, &g_objs.vao);
	glBindVertexArray(g_objs.vao);


	glGenBuffers(1, &g_objs.vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, g_objs.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &g_objs.indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_objs.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_index_buffer_data), g_index_buffer_data, GL_STATIC_DRAW);


	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);

	glBindVertexArray(0);

	
}
void UnoPlugin::Resize(void* backendData)
{
}
void UnoPlugin::Render(void* backendData)
{
	glViewport(0, 0, framebufferX, framebufferY);
	glClearColor(0.0f, 0.4f, 0.4f, 1.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glUseProgram(g_objs.shaderProgram);
	glBindVertexArray(g_objs.vao);
	
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

}