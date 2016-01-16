#include "Node.h"
#include "Director.h"
#include <iostream>

namespace mgrrenderer
{

// TODO:ここに置くのがあまりいいとは思わないがとりあえず
std::string ATTRIBUTE_NAME_POSITION = "a_position";
std::string ATTRIBUTE_NAME_COLOR = "a_color";
std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE = "a_texCoord";
std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_1 = "a_texCoord1";
std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_2 = "a_texCoord2";
std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_3 = "a_texCoord3";
std::string ATTRIBUTE_NAME_NORMAL = "a_normal";
std::string ATTRIBUTE_NAME_BLEND_WEIGHT = "a_blendWeight";
std::string ATTRIBUTE_NAME_BLEND_INDEX = "a_blendIndex";

Node::Node() : _scale(Vec3(1.0f, 1.0f, 1.0f)), _modelMatrix(Mat4::IDENTITY), _color(Color::WHITE)
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
	// 何もしない
}

void Node::render()
{
	// 何もしない
}

void Node::setRotation(const Vec3& angleVec) {
	_rotation = Quaternion(angleVec);
}

Mat4 Node::getRotationMatrix() const
{
	return Mat4::createRotation(_rotation);
}

OpenGLProgramData Node::createOpenGLProgram(const GLchar* vertexShaderSource, const GLchar* fragmentShaderSource)
{
	OpenGLProgramData ret;
	ret.vertexShader = createVertexShader(vertexShaderSource);
	ret.fragmentShader = createFragmentShader(fragmentShaderSource);
	ret.shaderProgram = createShaderProgram(ret.vertexShader, ret.fragmentShader);

	ret.uniformModelMatrix = glGetUniformLocation(ret.shaderProgram, "u_modelMatrix");
	ret.uniformViewMatrix = glGetUniformLocation(ret.shaderProgram, "u_viewMatrix");
	ret.uniformProjectionMatrix = glGetUniformLocation(ret.shaderProgram, "u_projectionMatrix");

	return ret;
}

void Node::destroyOpenGLProgram(const OpenGLProgramData& programData) const
{
	glUseProgram(0);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL処理でエラー発生 glGetError()=%d", glGetError());

	glDeleteProgram(programData.shaderProgram);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL処理でエラー発生 glGetError()=%d", glGetError());

	Logger::logAssert(glIsProgram(programData.shaderProgram) == GL_FALSE, "シェーダプログラムが既に破棄されている");

	glDeleteShader(programData.vertexShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL処理でエラー発生 glGetError()=%d", glGetError());

	glDeleteShader(programData.fragmentShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL処理でエラー発生 glGetError()=%d", glGetError());
}

GLuint Node::createVertexShader(const GLchar* source) const
{
	GLuint ret = glCreateShader(GL_VERTEX_SHADER);
	Logger::logAssert(ret != 0, "シェーダ作成失敗");
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL処理でエラー発生 glGetError()=%d", glGetError());
	if (ret == 0)
	{
		return ret;
	}

	GLint compileResult = compileShader(ret, source);
	Logger::logAssert(compileResult == GL_TRUE, "コンパイル失敗");
	if (compileResult == GL_FALSE)
	{
		return 0;
	}

	return ret;
}

GLuint Node::createFragmentShader(const GLchar* source) const
{
	GLuint ret = glCreateShader(GL_FRAGMENT_SHADER);
	Logger::logAssert(ret != 0, "シェーダプログラム作成失敗。");
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL処理でエラー発生 glGetError()=%d", glGetError());
	if (ret == 0)
	{
		return ret;
	}

	GLint compileResult = compileShader(ret, source);
	Logger::logAssert(compileResult == GL_TRUE, "コンパイル失敗");
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
	Logger::logAssert(ret != 0, "シェーダプログラム生成失敗");
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL処理でエラー発生 glGetError()=%d", glGetError());
	if (ret == 0)
	{
		return 0;
	}

	glAttachShader(ret, vertexShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL処理でエラー発生 glGetError()=%d", glGetError());
	glAttachShader(ret, fragmentShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL処理でエラー発生 glGetError()=%d", glGetError());

	// 固定のattribute変数文字列を固定のattribute変数IDに結びつける
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
	Logger::logAssert(linkResult == GL_TRUE, "シェーダリンク失敗。");
	if (linkResult == GL_FALSE)
	{
		return 0;
	}
	return ret;
}

} // namespace mgrrenderer
