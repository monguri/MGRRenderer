#pragma once

#include "Config.h"

#if defined(MGRRENDERER_USE_OPENGL)
#define GLEW_STATIC
#include <glew/include/glew.h>
#include <unordered_map>

namespace mgrrenderer
{

class GLProgram final
{
public:
	// TODO:‚±‚±‚É’u‚­‚Ì‚ª‚ ‚Ü‚è‚¢‚¢‚Æ‚ÍŽv‚í‚È‚¢‚ª‚Æ‚è‚ ‚¦‚¸
	static const std::string GLProgram::UNIFORM_NAME_MULTIPLE_COLOR;
	static const std::string GLProgram::UNIFORM_NAME_TEXTURE_SAMPLER;
	static const std::string GLProgram::UNIFORM_NAME_MODEL_MATRIX;
	static const std::string GLProgram::UNIFORM_NAME_VIEW_MATRIX;
	static const std::string GLProgram::UNIFORM_NAME_NORMAL_MATRIX;
	static const std::string GLProgram::UNIFORM_NAME_PROJECTION_MATRIX;
	static const std::string GLProgram::UNIFORM_NAME_CUBEMAP_FACE;

	enum class AttributeLocation : int
	{
		NONE = -1,

		POSITION,
		COLOR,
		TEXTURE_COORDINATE,
		TEXTURE_COORDINATE_1,
		TEXTURE_COORDINATE_2,
		TEXTURE_COORDINATE_3,
		NORMAL,
		BLEND_WEIGHT,
		BLEND_INDEX,

		NUM_ATTRIBUTE_IDS,
	};

	GLProgram();
	~GLProgram();
	static void checkGLError();
	void initWithShaderString(const GLchar* vertexShaderStr, const GLchar* fragmentShaderStr);
	void initWithShaderFile(const std::string& vertexShaderFile, const std::string& fragmentShaderFile);
	GLuint getAttributeLocation(const std::string& attributeName) const;
	GLint getUniformLocation(const std::string& uniformName) const;
	GLuint getShaderProgram() { return _shaderProgram; }

private:
	static GLenum _glError;

	GLuint _vertexShader;
	GLuint _fragmentShader;
	GLuint _shaderProgram;

	std::unordered_map<std::string, GLuint> _attributeList;
	std::unordered_map<std::string, GLint> _uniformList;

	GLuint createVertexShader(const GLchar* source) const;
	GLuint createFragmentShader(const GLchar* source) const;
	GLuint createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader);
	GLint compileShader(GLuint shader, const GLchar* source) const;
	void parseAttributes(GLuint shaderProgram);
	void parseUniforms(GLuint shaderProgram);
};

} // namespace mgrrenderer
#endif
