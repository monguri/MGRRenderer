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
	// ‚±‚ÌêŠ‚É‚ ‚é‚Ì‚Í•Ï‚¾‚ªAUtilityƒNƒ‰ƒX‚ÍŒã‚Åì‚ë‚¤
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
