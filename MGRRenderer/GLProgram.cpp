#include "GLProgram.h"
#if defined(MGRRENDERER_USE_OPENGL)
#include "Logger.h"
#include "FileUtility.h"
#include <string>

namespace mgrrenderer
{

// TODO:ここに置くのがあまりいいとは思わないがとりあえず
static const std::string ATTRIBUTE_NAME_POSITION = "a_position";
static const std::string ATTRIBUTE_NAME_COLOR = "a_color";
static const std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE = "a_texCoord";
static const std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_1 = "a_texCoord1";
static const std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_2 = "a_texCoord2";
static const std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_3 = "a_texCoord3";
static const std::string ATTRIBUTE_NAME_NORMAL = "a_normal";
static const std::string ATTRIBUTE_NAME_BLEND_WEIGHT = "a_blendWeight";
static const std::string ATTRIBUTE_NAME_BLEND_INDEX = "a_blendIndex";

const std::string GLProgram::GLProgram::UNIFORM_NAME_MULTIPLE_COLOR = "u_multipleColor";
const std::string GLProgram::GLProgram::UNIFORM_NAME_TEXTURE_SAMPLER = "u_texture";
const std::string GLProgram::GLProgram::UNIFORM_NAME_MODEL_MATRIX = "u_modelMatrix";
const std::string GLProgram::GLProgram::UNIFORM_NAME_VIEW_MATRIX = "u_viewMatrix";
const std::string GLProgram::GLProgram::UNIFORM_NAME_NORMAL_MATRIX = "u_normalMatrix";
const std::string GLProgram::GLProgram::UNIFORM_NAME_PROJECTION_MATRIX = "u_projectionMatrix";

GLProgram::GLProgram() :
_vertexShader(0),
_fragmentShader(0),
_shaderProgram(0)
{
}

GLProgram::~GLProgram()
{
	glUseProgram(0);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	if (_shaderProgram > 0)
	{
		glDeleteProgram(_shaderProgram);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	}

	if (_vertexShader > 0)
	{
		glDeleteShader(_vertexShader);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	}

	if (_fragmentShader > 0)
	{
		glDeleteShader(_fragmentShader);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	}
}

void GLProgram::initWithShaderString(const GLchar* vertexShaderStr, const GLchar* fragmentShaderStr)
{
	_vertexShader = createVertexShader(vertexShaderStr);
	_fragmentShader = createFragmentShader(fragmentShaderStr);
	_shaderProgram = createShaderProgram(_vertexShader, _fragmentShader);

	// この時点でバーテックスシェーダとフラグメントシェーダオブジェクトは解放
	if (_vertexShader > 0)
	{
		glDeleteShader(_vertexShader);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	}

	if (_fragmentShader > 0)
	{
		glDeleteShader(_fragmentShader);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	}

	_vertexShader = _fragmentShader = 0;

	// GL_ACTIVE_XX系はリンクしてプログラムが完成した後でないと取得失敗する
	parseAttributes(_shaderProgram);
	parseUniforms(_shaderProgram);
}

void GLProgram::initWithShaderFile(const std::string& vertexShaderFile, const std::string& fragmentShaderFile)
{
	const std::string& vertexShaderStr = FileUtility::getInstance()->getStringFromFile(vertexShaderFile);
	const std::string& fragmentShaderStr = FileUtility::getInstance()->getStringFromFile(fragmentShaderFile);
	return initWithShaderString(vertexShaderStr.c_str(), fragmentShaderStr.c_str());
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

GLuint GLProgram::createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader)
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

void GLProgram::parseAttributes(GLuint shaderProgram)
{
	_attributeList.clear();
	GLint numAttribute;
	glGetProgramiv(shaderProgram, GL_ACTIVE_ATTRIBUTES, &numAttribute);

	if (numAttribute > 0)
	{
		GLint maxLength;
		glGetProgramiv(shaderProgram, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);

		if (maxLength > 0)
		{
			GLchar* attributeName = (GLchar*)alloca(maxLength + 1);

			for (int i = 0; i < numAttribute; ++i)
			{
				GLint size;
				GLenum type;
				GLsizei length;
				glGetActiveAttrib(shaderProgram, i, maxLength, &length, &size, &type, attributeName);
				attributeName[length] = '\0';

				GLint location = glGetAttribLocation(shaderProgram, attributeName);
				Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
				_attributeList[attributeName] = location;
			}
		}
	}
	else
	{
		GLchar errorLog[1024];
		glGetProgramInfoLog(shaderProgram, sizeof(errorLog), nullptr, errorLog);
		Logger::log("アクティブなアトリビュート変数が取得できず。errorMsg=%s", errorLog);
	}
}

void GLProgram::parseUniforms(GLuint shaderProgram)
{
	_uniformList.clear();
	GLint numUniform;
	glGetProgramiv(shaderProgram, GL_ACTIVE_UNIFORMS, &numUniform);

	if (numUniform > 0)
	{
		GLint maxLength;
		glGetProgramiv(shaderProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

		if (maxLength > 0)
		{
			GLchar* uniformName = (GLchar*)alloca(maxLength + 1);

			for (int i = 0; i < numUniform; ++i)
			{
				GLint size;
				GLenum type;
				GLsizei length;
				glGetActiveUniform(shaderProgram, i, maxLength, &length, &size, &type, uniformName);
				uniformName[length] = '\0';

				if (length > 3)
				{
					// 配列型変数は[]以前の変数名を用いる
					char* c = strrchr(uniformName, '[');
					if (c != nullptr)
					{
						*c = '\0';
					}
				}

				GLint location = glGetUniformLocation(shaderProgram, uniformName);
				Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
				_uniformList[uniformName] = location;
			}
		}
	}
	else
	{
		GLchar errorLog[1024];
		glGetProgramInfoLog(shaderProgram, sizeof(errorLog), nullptr, errorLog);
		Logger::log("アクティブなユニフォーム変数が取得できず。errorMsg=%s", errorLog);
	}
}

GLuint GLProgram::getAttributeLocation(const std::string& attributeName) const
{
	try
	{
		return _attributeList.at(attributeName);
	}
	catch (...)
	{
		Logger::logAssert(false, "存在しないアトリビュート変数へのアクセス uniformName=%s", attributeName.c_str());
		return 0;
	}
}

GLint GLProgram::getUniformLocation(const std::string& uniformName) const
{
	try
	{
		return _uniformList.at(uniformName);
	}
	catch (...)
	{
		Logger::logAssert(false, "存在しないユニフォーム変数へのアクセス uniformName=%s", uniformName.c_str());
		return -1;
	}
}

} // namespace mgrrenderer

#endif
