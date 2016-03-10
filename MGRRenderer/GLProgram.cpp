#include "GLProgram.h"
#include "Logger.h"
#include <string>

namespace mgrrenderer
{

// TODO:�����ɒu���̂����܂肢���Ƃ͎v��Ȃ����Ƃ肠����
static const std::string ATTRIBUTE_NAME_POSITION = "a_position";
static const std::string ATTRIBUTE_NAME_COLOR = "a_color";
static const std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE = "a_texCoord";
static const std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_1 = "a_texCoord1";
static const std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_2 = "a_texCoord2";
static const std::string ATTRIBUTE_NAME_TEXTURE_COORDINATE_3 = "a_texCoord3";
static const std::string ATTRIBUTE_NAME_NORMAL = "a_normal";
static const std::string ATTRIBUTE_NAME_BLEND_WEIGHT = "a_blendWeight";
static const std::string ATTRIBUTE_NAME_BLEND_INDEX = "a_blendIndex";

const std::string UNIFORM_NAME_MULTIPLE_COLOR = "u_multipleColor";
const std::string UNIFORM_NAME_TEXTURE_SAMPLER = "u_texture";
const std::string UNIFORM_NAME_MODEL_MATRIX = "u_modelMatrix";
const std::string UNIFORM_NAME_VIEW_MATRIX = "u_viewMatrix";
const std::string UNIFORM_NAME_NORMAL_MATRIX = "u_normalMatrix";
const std::string UNIFORM_NAME_PROJECTION_MATRIX = "u_projectionMatrix";

GLProgram::~GLProgram()
{
	glUseProgram(0);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glDeleteProgram(shaderProgram);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	Logger::logAssert(glIsProgram(shaderProgram) == GL_FALSE, "�V�F�[�_�v���O���������ɔj������Ă���");

	glDeleteShader(vertexShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glDeleteShader(fragmentShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
}

void GLProgram::initWithShaderString(const GLchar* vertexShaderStr, const GLchar* fragmentShaderStr)
{
	vertexShader = createVertexShader(vertexShaderStr);
	fragmentShader = createFragmentShader(fragmentShaderStr);
	shaderProgram = createShaderProgram(vertexShader, fragmentShader);
}

GLuint GLProgram::createVertexShader(const GLchar* source) const
{
	GLuint ret = glCreateShader(GL_VERTEX_SHADER);
	Logger::logAssert(ret != 0, "�V�F�[�_�쐬���s");
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	if (ret == 0)
	{
		return ret;
	}

	GLint compileResult = compileShader(ret, source);
	Logger::logAssert(compileResult == GL_TRUE, "�R���p�C�����s");
	if (compileResult == GL_FALSE)
	{
		return 0;
	}

	return ret;
}

GLuint GLProgram::createFragmentShader(const GLchar* source) const
{
	GLuint ret = glCreateShader(GL_FRAGMENT_SHADER);
	Logger::logAssert(ret != 0, "�V�F�[�_�v���O�����쐬���s�B");
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	if (ret == 0)
	{
		return ret;
	}

	GLint compileResult = compileShader(ret, source);
	Logger::logAssert(compileResult == GL_TRUE, "�R���p�C�����s");
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
	Logger::logAssert(ret != 0, "�V�F�[�_�v���O�����������s");
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	if (ret == 0)
	{
		return 0;
	}

	glAttachShader(ret, vertexShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	glAttachShader(ret, fragmentShader);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	// �Œ��attribute�ϐ���������Œ��attribute�ϐ�ID�Ɍ��т���
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
	Logger::logAssert(linkResult == GL_TRUE, "�V�F�[�_�����N���s�B");
	if (linkResult == GL_FALSE)
	{
		return 0;
	}


	// GL_ACTIVE_XX�n�̓����N���ăv���O����������������łȂ��Ǝ擾���s����
	_uniformList.clear();
	GLint numUniform;
	glGetProgramiv(ret, GL_ACTIVE_UNIFORMS, &numUniform);

	if (numUniform > 0)
	{
		GLint maxLength;
		glGetProgramiv(ret, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

		if (maxLength > 0)
		{
			GLchar* uniformName = (GLchar*)alloca(maxLength + 1);

			for (int i = 0; i < numUniform; ++i)
			{
				GLint size;
				GLenum type;
				GLsizei length;
				glGetActiveUniform(ret, i, maxLength, &length, &size, &type, uniformName);
				uniformName[length] = '\0';

				if (length > 3)
				{
					// �z��^�ϐ���[]�ȑO�̕ϐ�����p����
					char* c = strrchr(uniformName, '[');
					if (c != nullptr)
					{
						*c = '\0';
					}
				}

				GLint location = glGetUniformLocation(ret, uniformName);
				Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
				_uniformList[uniformName] = location;
			}
		}
	}
	else
	{
		GLchar errorLog[1024];
		glGetProgramInfoLog(ret, sizeof(errorLog), nullptr, errorLog);
		Logger::log("�A�N�e�B�u�ȃ��j�t�H�[���ϐ����擾�ł����BerrorMsg=%s", errorLog);
	}

	return ret;
}

GLint GLProgram::getUniformLocation(const std::string& uniformName) const
{
	try
	{
		return _uniformList.at(uniformName);
	}
	catch (...)
	{
		Logger::logAssert(false, "���݂��Ȃ����j�t�H�[���ϐ��ւ̃A�N�Z�X uniformName=%s", uniformName.c_str());
		return -1;
	}
}

} // namespace mgrrenderer
