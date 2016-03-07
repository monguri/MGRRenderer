#include "GLProgram.h"
#include "Logger.h"
#include <string>

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

GLProgram::~GLProgram()
{
	glUseProgram(0);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glDeleteProgram(shaderProgram);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	Logger::logAssert(glIsProgram(shaderProgram) == GL_FALSE, "シェーダプログラムが既に破棄されている");

	glDeleteShader(vertexShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glDeleteShader(fragmentShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
}

void GLProgram::initWithShaderString(const GLchar* vertexShaderStr, const GLchar* fragmentShaderStr)
{
	vertexShader = createVertexShader(vertexShaderStr);
	fragmentShader = createFragmentShader(fragmentShaderStr);
	shaderProgram = createShaderProgram(vertexShader, fragmentShader);

	uniformMultipleColor = glGetUniformLocation(shaderProgram, "u_multipleColor");
	uniformModelMatrix = glGetUniformLocation(shaderProgram, "u_modelMatrix");
	uniformViewMatrix = glGetUniformLocation(shaderProgram, "u_viewMatrix");
	uniformProjectionMatrix = glGetUniformLocation(shaderProgram, "u_projectionMatrix");
}

GLuint GLProgram::createVertexShader(const GLchar* source) const
{
	GLuint ret = glCreateShader(GL_VERTEX_SHADER);
	Logger::logAssert(ret != 0, "シェーダ作成失敗");
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
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

GLuint GLProgram::createFragmentShader(const GLchar* source) const
{
	GLuint ret = glCreateShader(GL_FRAGMENT_SHADER);
	Logger::logAssert(ret != 0, "シェーダプログラム作成失敗。");
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
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

GLint GLProgram::compileShader(GLuint shader, const GLchar* source) const
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
			Logger::log(errorMsg);
			free(errorMsg);
		}
		else
		{
			Logger::log("compile error but no info.");
		}
	}

	return compileResult;
}

GLuint GLProgram::createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader) const
{
	GLuint ret = glCreateProgram();
	Logger::logAssert(ret != 0, "シェーダプログラム生成失敗");
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	if (ret == 0)
	{
		return 0;
	}

	glAttachShader(ret, vertexShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glAttachShader(ret, fragmentShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

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
			Logger::log(errorMsg);
			free(errorMsg);
		}
		else
		{
			Logger::log("link error but no info.");
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
