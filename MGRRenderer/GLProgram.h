#pragma once
#include <gles/include/glew.h>

namespace mgrrenderer
{

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
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint shaderProgram;
	GLint uniformMultipleColor;
	GLint uniformTexture;
	GLint uniformCameraPosition;
	GLint uniformModelMatrix;
	GLint uniformViewMatrix;
	GLint uniformNormalMatrix;
	GLint uniformProjectionMatrix;
	GLint uniformSkinMatrixPalette;
	GLint uniformAmbientLightColor;
	GLint uniformDirectionalLightColor;
	GLint uniformDirectionalLightDirection;
	GLint uniformPointLightColor;
	GLint uniformPointLightPosition;
	GLint uniformPointLightRangeInverse;
	GLint uniformSpotLightColor;
	GLint uniformSpotLightPosition;
	GLint uniformSpotLightDirection;
	GLint uniformSpotLightRangeInverse;
	GLint uniformSpotLightInnerAngleCos;
	GLint uniformSpotLightOuterAngleCos;
	GLint uniformMaterialAmbient;
	GLint uniformMaterialDiffuse;
	GLint uniformMaterialSpecular;
	GLint uniformMaterialEmissive;
	GLint uniformMaterialOpacity;
	GLint uniformMaterialShininess;

	~GLProgram();
	void initWithShaderString(const GLchar* vertexShaderStr, const GLchar* fragmentShaderStr);
	GLuint createVertexShader(const GLchar* source) const;
	GLuint createFragmentShader(const GLchar* source) const;
	GLuint createShaderProgram(const GLuint vertexShader, const GLuint fragmentShader) const;
	GLint compileShader(GLuint shader, const GLchar* source) const;
};

} // namespace mgrrenderer
