#include "Node.h"
#include "Director.h"
#include <iostream>
#include <assert.h>

namespace mgrrenderer
{

void Node::visit()
{
	render();
}

void Node::render()
{
	// ‰½‚à‚µ‚È‚¢
}

OpenGLProgramData Node::createOpenGLProgram(const GLchar* vertexShaderSource, const GLchar* fragmentShaderSource)
{
	OpenGLProgramData ret;
	ret.vertexShader = createVertexShader(vertexShaderSource);
	ret.fragmentShader = createFragmentShader(fragmentShaderSource);
	ret.shaderProgram = createShaderProgram(ret.vertexShader, ret.fragmentShader);

	ret.attributeVertexPosition = glGetAttribLocation(ret.shaderProgram, "attr_pos");
	assert(glGetError() == GL_NO_ERROR);
	assert(ret.attributeVertexPosition >= 0);

	ret.uniformViewMatrix = glGetUniformLocation(ret.shaderProgram, "unif_view_mat");
	ret.uniformProjectionMatrix = glGetUniformLocation(ret.shaderProgram, "unif_proj_mat");

	return ret;
}

void Node::destroyOpenGLProgram(const OpenGLProgramData& programData) const
{
	glUseProgram(0);
	assert(glGetError() == GL_NO_ERROR);

	glDeleteProgram(programData.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	assert(glIsProgram(programData.shaderProgram) == GL_FALSE);

	glDeleteShader(programData.vertexShader);
	assert(glGetError() == GL_NO_ERROR);

	glDeleteShader(programData.fragmentShader);
	assert(glGetError() == GL_NO_ERROR);
}

GLuint Node::createVertexShader(const GLchar* source) const
{
	GLuint ret = glCreateShader(GL_VERTEX_SHADER);
	assert(ret != 0);
	assert(glGetError() == GL_NO_ERROR);
	if (ret == 0)
	{
		return ret;
	}

	GLint compileResult = compileShader(ret, source);
	assert(compileResult == GL_TRUE);
	if (compileResult == GL_FALSE)
	{
		return 0;
	}

	return ret;
}

GLuint Node::createFragmentShader(const GLchar* source) const
{
	GLuint ret = glCreateShader(GL_FRAGMENT_SHADER);
	assert(ret != 0);
	assert(glGetError() == GL_NO_ERROR);
	if (ret == 0)
	{
		return ret;
	}

	GLint compileResult = compileShader(ret, source);
	assert(compileResult == GL_TRUE);
	if (compileResult == GL_FALSE)
	{
		return 0;
	}

	return ret;
}

GLint Node::compileShader(GLuint shader, const GLchar* source) const
{
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint compileResult;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);
	if (compileResult == GL_FALSE)
	{
		GLint errorLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &errorLen);
		if (errorLen> 1)
		{
			GLchar* errorMsg = (GLchar*)calloc(errorLen, sizeof(GLchar));
			glGetShaderInfoLog(shader, errorLen, nullptr, errorMsg);
			std::cout << errorMsg;
			free(errorMsg);
		}
		else
		{
			std::cout << "compile error but no info.";
		}
	}

	return compileResult;
}

GLuint Node::createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader) const
{
	GLuint ret = glCreateProgram();
	assert(ret != 0);
	assert(glGetError() == GL_NO_ERROR);
	if (ret == 0)
	{
		return 0;
	}

	glAttachShader(ret, vertexShader);
	assert(glGetError() == GL_NO_ERROR);
	glAttachShader(ret, fragmentShader);
	assert(glGetError() == GL_NO_ERROR);

	glLinkProgram(ret);

	GLint linkResult;
	glGetProgramiv(ret, GL_LINK_STATUS, &linkResult);
	if (linkResult == GL_FALSE)
	{
		GLint errorLen;
		glGetProgramiv(ret, GL_INFO_LOG_LENGTH, &errorLen);
		if (errorLen> 1)
		{
			GLchar* errorMsg = (GLchar*)calloc(errorLen, sizeof(GLchar));
			glGetProgramInfoLog(ret, errorLen, nullptr, errorMsg);
			std::cout << errorMsg;
			free(errorMsg);
		}
		else
		{
			std::cout << "link error but no info.";
		}
	}
	assert(linkResult == GL_TRUE);
	if (linkResult == GL_FALSE)
	{
		return 0;
	}
	return ret;
}

} // namespace mgrrenderer
