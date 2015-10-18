#pragma once
#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include "BasicDataTypes.h"

namespace mgrrenderer
{

class Node
{
public:
	Node();
	~Node();
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
