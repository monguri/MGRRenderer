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
	//GLint uniformMultipleColor;
	//GLint uniformTexture;
	//GLint uniformCameraPosition;
	//GLint uniformModelMatrix;
	//GLint uniformViewMatrix;
	//GLint uniformNormalMatrix;
	//GLint uniformProjectionMatrix;
	//GLint uniformSkinMatrixPalette;
	//GLint uniformAmbientLightColor;
	//GLint uniformDirectionalLightColor;
	//GLint uniformDirectionalLightDirection;
	//GLint uniformPointLightColor;
	//GLint uniformPointLightPosition;
	//GLint uniformPointLightRangeInverse;
	//GLint uniformSpotLightColor;
	//GLint uniformSpotLightPosition;
	//GLint uniformSpotLightDirection;
	//GLint uniformSpotLightRangeInverse;
	//GLint uniformSpotLightInnerAngleCos;
	//GLint uniformSpotLightOuterAngleCos;
	//GLint uniformMaterialAmbient;
	//GLint uniformMaterialDiffuse;
	//GLint uniformMaterialSpecular;
	//GLint uniformMaterialEmissive;
	//GLint uniformMaterialOpacity;
	//GLint uniformMaterialShininess;
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint shaderProgram;

	~GLProgram();
	void initWithShaderString(const GLchar* vertexShaderStr, const GLchar* fragmentShaderStr);
	GLint getUniformLocation(const std::string& uniformName) const;

private:
	std::unordered_map<std::string, GLint> _uniformList;

	GLuint createVertexShader(const GLchar* source) const;
	GLuint createFragmentShader(const GLchar* source) const;
	GLuint createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader);
	GLint compileShader(GLuint shader, const GLchar* source) const;
};

} // namespace mgrrenderer
