#include "Node.h"
#include "Director.h"
#include <iostream>
#include <assert.h>

namespace mgrrenderer
{

// TODO:‚±‚±‚É’u‚­‚Ì‚ª‚ ‚Ü‚è‚¢‚¢‚Æ‚ÍŽv‚í‚È‚¢‚ª‚Æ‚è‚ ‚¦‚¸
std::string ATTRIBUTE_NAME_POSITION = "attr_position";
std::string ATTRIBUTE_NAME_COLOR = "attr_color";
std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE = "attr_texCoord";
std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_1 = "attr_texCoord1";
std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_2 = "attr_texCoord2";
std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_3 = "attr_texCoord3";
std::string ATTRIBUTE_NAME_NORMAL = "attr_normal";
std::string ATTRIBUTE_NAME_BLEND_WEIGHT = "attr_blendWeight";
std::string ATTRIBUTE_NAME_BLEND_INDEX = "attr_blendIndex";

Node::Node() : _scale(Vec3(1.0f, 1.0f, 1.0f)), _modelMatrix(Mat4::IDENTITY)
{
}

void Node::visit(float dt)
{
	update(dt);

	_modelMatrix = Mat4::createTransform(_position, _rotation, _scale);

	render();
}

void Node::update(float dt)
{
	// ‰½‚à‚µ‚È‚¢
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

	ret.uniformModelMatrix = glGetUniformLocation(ret.shaderProgram, "unif_modelMatrix");
	ret.uniformViewMatrix = glGetUniformLocation(ret.shaderProgram, "unif_viewMatrix");
	ret.uniformProjectionMatrix = glGetUniformLocation(ret.shaderProgram, "unif_projectionMatrix");

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

	// ŒÅ’è‚Ìattribute•Ï”•¶Žš—ñ‚ðŒÅ’è‚Ìattribute•Ï”ID‚ÉŒ‹‚Ñ‚Â‚¯‚é
	static const struct Attribute {
		const std::string& name;
		AttributeLocation location;
	} attributes[] =
	{
		{ATTRIBUTE_NAME_POSITION, AttributeLocation::POSITION},
		{ATTRIBUTE_NAME_COLOR, AttributeLocation::COLOR},
		{ATTRIBUTE_NAME_TEXTURE_COORDINATE, AttributeLocation::TEXTURE_COORDINATE},
		{ATTRIBUTE_NAME_TEXTURE_COORDINATE_1, AttributeLocation::TEXTURE_COORDINATE_1},
		{ATTRIBUTE_NAME_TEXTURE_COORDINATE_2, AttributeLocation::TEXTURE_COORDINATE_2},
		{ATTRIBUTE_NAME_TEXTURE_COORDINATE_3, AttributeLocation::TEXTURE_COORDINATE_3},
		{ATTRIBUTE_NAME_NORMAL, AttributeLocation::NORMAL},
		{ATTRIBUTE_NAME_BLEND_WEIGHT, AttributeLocation::BLEND_WEIGHT},
		{ATTRIBUTE_NAME_BLEND_INDEX, AttributeLocation::BLEND_INDEX},
	};

	for (const Attribute& attribute : attributes)
	{
		glBindAttribLocation(ret, (GLuint)attribute.location, attribute.name.c_str());
	}

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
