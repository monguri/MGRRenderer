#pragma once
#include <gles/include/glew.h>
#include <unordered_map>

namespace mgrrenderer
{

// TODO:‚±‚±‚É’u‚­‚Ì‚ª‚ ‚Ü‚è‚¢‚¢‚Æ‚ÍŽv‚í‚È‚¢‚ª‚Æ‚è‚ ‚¦‚¸
extern const std::string UNIFORM_NAME_MULTIPLE_COLOR;
extern const std::string UNIFORM_NAME_TEXTURE_SAMPLER;
extern const std::string UNIFORM_NAME_MODEL_MATRIX;
extern const std::string UNIFORM_NAME_VIEW_MATRIX;
extern const std::string UNIFORM_NAME_NORMAL_MATRIX;
extern const std::string UNIFORM_NAME_PROJECTION_MATRIX;

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

class GLProgram final
{
public:
	GLProgram();
	~GLProgram();
	void initWithShaderString(const GLchar* vertexShaderStr, const GLchar* fragmentShaderStr);
	GLuint getAttributeLocation(const std::string& attributeName) const;
	GLint getUniformLocation(const std::string& uniformName) const;
	GLuint getShaderProgram() { return _shaderProgram; }

private:
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
