#pragma once
#include <GLFW/glfw3.h>

namespace mgrrenderer
{

struct Vec2
{
	GLfloat x;
	GLfloat y;

	Vec2(GLfloat x, GLfloat y) : x(x), y(y) {}
};

struct OpenGLProgramData
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint shaderProgram;
	GLint attributeVertexPosition;
};

} // namespace mgrrenderer
