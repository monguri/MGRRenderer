#pragma once
#include <gles/include/glew.h>
#include <glfw3/include/glfw3.h>
#include "BasicDataTypes.h"

namespace mgrrenderer
{

class Node
{
public:
	virtual void visit();

protected:
	// この場所にあるのは変だが、Utilityクラスは後で作ろう
	OpenGLProgramData createOpenGLProgram(const GLchar* vertexShaderSource, const GLchar* fragmentShaderSource);
	void destroyOpenGLProgram(const OpenGLProgramData& programData) const;

	GLuint createVertexShader(const GLchar* source) const;
	GLuint createFragmentShader(const GLchar* source) const;
	GLuint createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader) const;

private:
	virtual void render();
	GLint compileShader(GLuint shader, const GLchar* source) const;
};

} // namespace mgrrenderer
