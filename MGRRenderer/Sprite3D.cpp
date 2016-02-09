#include "Sprite3D.h"
#include "ObjLoader.h"
#include "C3bLoader.h"
#include "Image.h"
#include "Director.h"
#include "Light.h"

namespace mgrrenderer
{

Sprite3D::Sprite3D() :
_isObj(false),
_isC3b(false),
_texture(nullptr),
_meshDatas(nullptr),
_nodeDatas(nullptr),
_perVertexByteSize(0),
_animationDatas(nullptr),
_currentAnimation(nullptr),
_loopAnimation(false),
_elapsedTime(0.0f)
{
}

Sprite3D::~Sprite3D()
{

	_currentAnimation = nullptr;

	if (_animationDatas != nullptr)
	{
		delete _animationDatas;
		_animationDatas = nullptr;
	}

	if (_nodeDatas != nullptr)
	{
		delete _nodeDatas;
		_nodeDatas = nullptr;
	}

	if (_meshDatas != nullptr)
	{
		delete _meshDatas;
		_meshDatas = nullptr;
	}

	if (_texture != nullptr)
	{
		delete _texture;
		_texture = nullptr;
	}
}

bool Sprite3D::initWithModel(const std::string& filePath)
{
	_isObj = false;
	_isC3b = false;

	const std::string& ext = filePath.substr(filePath.length() - 4, 4);
	if (ext == ".obj")
	{
		_isObj = true;

		std::vector<ObjLoader::MeshData> meshList;
		std::vector<ObjLoader::MaterialData> materialList;

		const std::string& err = ObjLoader::loadObj(filePath, meshList, materialList);
		if (!err.empty())
		{
			Logger::log(err.c_str());
			return false;
		}

		_vertices.clear();

		// materialList�͌��󖳎�
		// TODO:��������MeshData�͂��̎��_�Ń}�e���A�����Ƃɂ܂Ƃ܂��ĂȂ��̂ł́HfaceGroup������܂Ƃ܂��Ă�H
		// ���܂Ƃ܂��Ă�B�������A����̓}�e���A���͈��ނƂ����O��ł�����
		// �{���́Astd::vector<std::vector<Position3DTextureCoordinates>> �������o�ϐ��ɂȂ��ĂāA�}�e���A�����Ƃɐ؂�ւ��ĕ`�悷��
		// �e�N�X�`�����{���͐؂�ւ��O�񂾂���setTexture���ă��\�b�h����������ȁB�B
		Logger::logAssert(meshList.size() == 1, "���󃁃b�V�������ɂ͑Ή����ĂȂ��B");
		const ObjLoader::MeshData& mesh = meshList[0];
		_vertices = mesh.vertices;
		_indices = mesh.indices;
	}
	else if (ext == ".c3t" || ext == ".c3b")
	{
		_isC3b = true; // TODO:���̃t���O�����͔��ɂ�����

		_meshDatas = new (std::nothrow)C3bLoader::MeshDatas();
		C3bLoader::MaterialDatas* materialDatas = new (std::nothrow)C3bLoader::MaterialDatas();
		_nodeDatas = new (std::nothrow)C3bLoader::NodeDatas();
		_animationDatas = new (std::nothrow)C3bLoader::AnimationDatas();
		std::string err;
		if (ext == ".c3t")
		{
			err = C3bLoader::loadC3t(filePath, *_meshDatas, *materialDatas, *_nodeDatas, *_animationDatas);
		}
		else
		{
			Logger::logAssert(ext == ".c3b", "");
			err = C3bLoader::loadC3b(filePath, *_meshDatas, *materialDatas, *_nodeDatas, *_animationDatas);
		}

		if (!err.empty())
		{
			Logger::log(err.c_str());
			return false;
		}

		C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
		_indices = meshData->subMeshIndices[0];

		_perVertexByteSize = 0;

		for (C3bLoader::MeshVertexAttribute attrib : meshData->attributes)
		{
			_perVertexByteSize += attrib.attributeSizeBytes;
		}

		C3bLoader::MaterialData* materialData = materialDatas->materialDatas[0];
		const C3bLoader::TextureData& texture = materialData->textures[0];
		setTexture(texture.fileName);
		_ambient = materialData->ambient;
		_diffuse = materialData->diffuse;
		_specular = materialData->specular;
		//_emissive = materialData->emissive;
		//_opacity = materialData->opacity;
		_shininess = materialData->shininess;

		delete materialDatas;
	}
	else
	{
		Logger::logAssert(false, "�Ή����ĂȂ��g���q%s", ext);
	}

	if (_isObj)
	{
		_glData = createOpenGLProgram(
			// vertex shader
			// ModelData�����g��Ȃ��ꍇ
			"attribute vec4 a_position;"
			"attribute vec4 a_normal;"
			"attribute vec2 a_texCoord;"
			"varying vec4 v_normal;"
			"varying vec2 v_texCoord;"
			"varying vec3 v_vertexToPointLightDirection;"
			"varying vec3 v_vertexToSpotLightDirection;"
			"uniform mat4 u_modelMatrix;"
			"uniform mat4 u_viewMatrix;"
			"uniform mat4 u_projectionMatrix;"
			"uniform mat4 u_normalMatrix;"
			"uniform vec3 u_pointLightPosition;"
			"uniform vec3 u_spotLightPosition;"
			"void main()"
			"{"
			"	vec4 worldPosition = u_modelMatrix * a_position;"
			"	v_vertexToPointLightDirection = u_pointLightPosition - worldPosition.xyz;"
			"	v_vertexToSpotLightDirection = u_spotLightPosition - worldPosition.xyz;"
			"	gl_Position = u_projectionMatrix * u_viewMatrix * worldPosition;"
			"	v_normal = u_normalMatrix * a_normal;" // scale�ϊ��ɑΉ����邽�߂Ƀ��f���s��̋t�s���]�u�������̂�p����
			"	v_texCoord = a_texCoord;"
			"	v_texCoord.y = 1.0 - v_texCoord.y;" // c3b�̎���ɂ�����
			"}"
			,
			// fragment shader
			"uniform sampler2D u_texture;"
			"uniform vec3 u_multipleColor;"
			"uniform vec3 u_ambientLightColor;"
			"uniform vec3 u_directionalLightColor;"
			"uniform vec3 u_directionalLightDirection;"
			"uniform vec3 u_pointLightColor;"
			"uniform float u_pointLightRangeInverse;"
			"uniform vec3 u_spotLightColor;"
			"uniform vec3 u_spotLightDirection;"
			"uniform float u_spotLightRangeInverse;"
			"uniform float u_spotLightInnerAngleCos;"
			"uniform float u_spotLightOuterAngleCos;"
			"varying vec4 v_normal;"
			"varying vec2 v_texCoord;"
			"varying vec3 v_vertexToPointLightDirection;"
			"varying vec3 v_vertexToSpotLightDirection;"
			""
			"vec3 computeLightedColor(vec3 normalVector, vec3 lightDirection, vec3 lightColor, float attenuation)"
			"{"
			"	float diffuse = max(dot(normalVector, lightDirection), 0.0);"
			"	vec3 diffuseColor = lightColor * diffuse * attenuation;"
			"	return diffuseColor;"
			"}"
			""
			"void main()"
			"{"
			"	vec4 combinedColor = vec4(u_ambientLightColor, 1.0);"
			""
			"	vec3 normal = normalize(v_normal.xyz);" // �f�[�^�`���̎��_��normalize����ĂȂ��@��������͗l
			"	combinedColor.rgb += computeLightedColor(normal, -u_directionalLightDirection, u_directionalLightColor, 1.0);"
			""
			"	vec3 dir = v_vertexToPointLightDirection * u_pointLightRangeInverse;"
			"	float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);"
			"	combinedColor.rgb += computeLightedColor(normal, normalize(v_vertexToPointLightDirection), u_pointLightColor, attenuation);"
			""
			"	dir = v_vertexToSpotLightDirection * u_spotLightRangeInverse;"
			"	attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);"
			"	vec3 vertexToSpotLightDirection = normalize(v_vertexToSpotLightDirection);"
			"	float spotCurrentAngleCos = dot(u_spotLightDirection, -vertexToSpotLightDirection);"
			"	attenuation *= smoothstep(u_spotLightOuterAngleCos, u_spotLightInnerAngleCos, spotCurrentAngleCos);"
			"	attenuation = clamp(attenuation, 0.0, 1.0);"
			"	combinedColor.rgb += computeLightedColor(normal, vertexToSpotLightDirection, u_spotLightColor, attenuation);"
			""
			"	gl_FragColor = texture2D(u_texture, v_texCoord) * vec4(u_multipleColor, 0.0) * combinedColor;" // �e�N�X�`���ԍ���0�݂̂ɑΉ�
			"}"
			);
	}
	else if (_isC3b)
	{
		_glData = createOpenGLProgram(
			// vertex shader
			// ModelData�����g��Ȃ��ꍇ
			//"attribute vec4 a_position;"
			//"attribute vec2 a_texCoord;"
			//"varying vec2 v_texCoord;"
			//"uniform mat4 u_modelMatrix;"
			//"uniform mat4 u_viewMatrix;"
			//"uniform mat4 u_projectionMatrix;"
			//"void main()"
			//"{"
			//"	gl_Position = u_projectionMatrix * u_viewMatrix * a_position;"
			//"	v_texCoord = a_texCoord;"
			//"	v_texCoord.y = 1.0 - v_texCoord.y;"
			//"}"
			//// �A�j���[�V�������g���ꍇ
			"attribute vec3 a_position;" // ���ꂪvec3�ɂȂ��Ă���̂ɒ��� TODO:�Ȃ��Ȃ̂��H
			"attribute vec3 a_normal;"
			"attribute vec2 a_texCoord;"
			"attribute vec4 a_blendWeight;"
			"attribute vec4 a_blendIndex;"
			""
			"const int SKINNING_JOINT_COUNT = 60;" // TODO:�Ȃ�60�܂łȂ̂��H
			""
			"uniform mat4 u_modelMatrix;"
			"uniform mat4 u_lightViewMatrix;" // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��r���[�s��
			"uniform mat4 u_viewMatrix;"
			"uniform mat4 u_projectionMatrix;"
			"uniform mat4 u_normalMatrix;" // scale�ϊ��ɑΉ����邽�߂Ƀ��f���s��̋t�s���]�u�������̂�p����
			"uniform vec3 u_pointLightPosition;"
			"uniform vec3 u_spotLightPosition;"
			"uniform vec3 u_cameraPosition;"
			"uniform mat4 u_matrixPalette[SKINNING_JOINT_COUNT];"
			""
			"varying vec4 v_lightPosition;"
			"varying vec4 v_normal;"
			"varying vec2 v_texCoord;"
			"varying vec3 v_vertexToPointLightDirection;"
			"varying vec3 v_vertexToSpotLightDirection;"
			"varying vec3 v_vertexToCameraDirection;"
			""
			"vec4 getPosition()"
			"{"
			"	mat4 skinMatrix = u_matrixPalette[int(a_blendIndex[0])] * a_blendWeight[0];"
			""
			"	if (a_blendWeight[1] > 0.0)"
			"	{"
			"		skinMatrix += u_matrixPalette[int(a_blendIndex[1])] * a_blendWeight[1];"
			""
			"		if (a_blendWeight[2] > 0.0)"
			"		{"
			"			skinMatrix += u_matrixPalette[int(a_blendIndex[2])] * a_blendWeight[2];"
			""
			"			if (a_blendWeight[3] > 0.0)"
			"			{"
			"				skinMatrix += u_matrixPalette[int(a_blendIndex[3])] * a_blendWeight[3];"
			"			}"
			"		}"
			"	}"
			""
			"	vec4 position = vec4(a_position, 1.0);"
			"	vec4 skinnedPosition = skinMatrix * position;"
			"	skinnedPosition.w = 1.0;"
			"	return skinnedPosition;"
			"}"
			""
			"void main()"
			"{"
			"	vec4 worldPosition = u_modelMatrix * getPosition();"
			"	v_vertexToPointLightDirection = u_pointLightPosition - worldPosition.xyz;"
			"	v_vertexToSpotLightDirection = u_spotLightPosition - worldPosition.xyz;"
			"	v_vertexToCameraDirection = u_cameraPosition - worldPosition.xyz;"
			"	gl_Position = u_projectionMatrix * u_viewMatrix * worldPosition;"
			"	vec4 normal = vec4(a_normal, 1.0);"
			"	v_normal = u_normalMatrix * normal;"
			"	v_texCoord = a_texCoord;"
			"	v_texCoord.y = 1.0 - v_texCoord.y;" // c3b�̎���ɂ�����
			"	v_lightPosition = u_projectionMatrix * u_lightViewMatrix * u_modelMatrix * getPosition();"
			"}"
			,
			// fragment shader
			"uniform sampler2D u_texture;"
			"uniform sampler2D u_shadowTexture;"
			"uniform vec3 u_multipleColor;"
			"uniform vec3 u_directionalLightColor;"
			"uniform vec3 u_directionalLightDirection;"
			"uniform vec3 u_ambientLightColor;"
			"uniform vec3 u_pointLightColor;"
			"uniform float u_pointLightRangeInverse;"
			"uniform vec3 u_spotLightColor;"
			"uniform vec3 u_spotLightDirection;"
			"uniform float u_spotLightRangeInverse;"
			"uniform float u_spotLightInnerAngleCos;"
			"uniform float u_spotLightOuterAngleCos;"
			"uniform vec3 u_materialAmbient;"
			"uniform vec3 u_materialDiffuse;"
			"uniform vec3 u_materialSpecular;"
			"uniform float u_materialShininess;"
			"uniform vec3 u_materialEmissive;"
			"uniform float u_materialOpacity;"
			"varying vec4 v_lightPosition;"
			"varying vec4 v_normal;"
			"varying vec2 v_texCoord;"
			"varying vec3 v_vertexToPointLightDirection;"
			"varying vec3 v_vertexToSpotLightDirection;"
			"varying vec3 v_vertexToCameraDirection;"
			""
			"vec3 computeLightedColor(vec3 normalVector, vec3 lightDirection, vec3 cameraDirection, vec3 lightColor, vec3 ambient, vec3 diffuse, vec3 specular, float shininess, float attenuation)"
			"{"
			"	vec3 ambientColor = lightColor * ambient * attenuation;"
			""
			"	float intensityPerUnitArea = max(dot(normalVector, lightDirection), 0.0);"
			""
			"	vec3 diffuseColor = lightColor * diffuse * intensityPerUnitArea * attenuation;"
			""
			"	vec3 reflectedLightDirection = reflect(-lightDirection, normalVector);"
			"	vec3 specularColor = vec3(0.0, 0.0, 0.0);"
			"	if (intensityPerUnitArea > 0.0)"
			"	{"
			"		specularColor = lightColor * specular * pow(max(dot(reflectedLightDirection, cameraDirection), 0.0), shininess);"
			"	}"
			""
			"	return ambientColor + diffuseColor + specularColor;"
			"}"
			""
			"void main()"
			"{"
			"	vec4 combinedColor = vec4(u_ambientLightColor, 1.0);"
			""
			"	vec3 normal = normalize(v_normal.xyz);" // �f�[�^�`���̎��_��normalize����ĂȂ��@��������͗l
			""
			"	vec3 cameraDirection = normalize(v_vertexToCameraDirection);"
			""
			"	combinedColor.rgb += computeLightedColor(normal, -u_directionalLightDirection, cameraDirection, u_directionalLightColor, u_materialAmbient, u_materialDiffuse, u_materialSpecular, u_materialShininess, 1.0);"
			""
			"	vec3 dir = v_vertexToPointLightDirection * u_pointLightRangeInverse;"
			"	float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);"
			"	combinedColor.rgb += computeLightedColor(normal, normalize(v_vertexToPointLightDirection), cameraDirection, u_pointLightColor, u_materialAmbient, u_materialDiffuse, u_materialSpecular, u_materialShininess, attenuation);"
			""
			"	dir = v_vertexToSpotLightDirection * u_spotLightRangeInverse;"
			"	attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);"
			"	vec3 vertexToSpotLightDirection = normalize(v_vertexToSpotLightDirection);"
			"	float spotCurrentAngleCos = dot(u_spotLightDirection, -vertexToSpotLightDirection);"
			"	attenuation *= smoothstep(u_spotLightOuterAngleCos, u_spotLightInnerAngleCos, spotCurrentAngleCos);"
			"	attenuation = clamp(attenuation, 0.0, 1.0);"
			"	combinedColor.rgb += computeLightedColor(normal, vertexToSpotLightDirection, cameraDirection, u_spotLightColor, u_materialAmbient, u_materialDiffuse, u_materialSpecular, u_materialShininess, attenuation);"
			""
			"	gl_FragColor = texture2D(u_texture, v_texCoord) * vec4(u_multipleColor, 1.0) * combinedColor;" // �e�N�X�`���ԍ���0�݂̂ɑΉ�
			""
			"	vec4 depthCheck = v_lightPosition / v_lightPosition.w;"
			"	depthCheck = depthCheck / 2.0 + 0.5;"
			"	float textureDepth = texture2D(u_shadowTexture, depthCheck.xy).z;"
			"	if (depthCheck.z > textureDepth + 0.0003)" // TODO:��ŏC��
			"	{"
			"		gl_FragColor.rgb *= 0.5;" // TODO:������萔������Ȃ�Ē��r���[�B��ŏC���B�l��0.5�ɂ��Ă���B
			"	}"
			"}"
			);
	}

	_glData.attributeTextureCoordinates = glGetAttribLocation(_glData.shaderProgram, "a_texCoord");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.attributeTextureCoordinates < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformTexture = glGetUniformLocation(_glData.shaderProgram, "u_texture");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformTexture < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	if (_isC3b)
	{
		_glData.uniformSkinMatrixPalette = glGetUniformLocation(_glData.shaderProgram, "u_matrixPalette");
		if (glGetError() != GL_NO_ERROR)
		{
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
			return false;
		}

		if (_glData.uniformSkinMatrixPalette < 0)
		{
			Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
			return false;
		}

		_uniformShadowTexture = glGetUniformLocation(_glData.shaderProgram, "u_shadowTexture");
		if (glGetError() != GL_NO_ERROR)
		{
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
			return false;
		}

		if (_uniformShadowTexture < 0)
		{
			Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
			return false;
		}
	}

	_glData.uniformAmbientLightColor = glGetUniformLocation(_glData.shaderProgram, "u_ambientLightColor");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformAmbientLightColor < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformDirectionalLightColor = glGetUniformLocation(_glData.shaderProgram, "u_directionalLightColor");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformDirectionalLightColor < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformDirectionalLightDirection = glGetUniformLocation(_glData.shaderProgram, "u_directionalLightDirection");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformDirectionalLightDirection < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformPointLightColor = glGetUniformLocation(_glData.shaderProgram, "u_pointLightColor");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformPointLightColor < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformPointLightPosition = glGetUniformLocation(_glData.shaderProgram, "u_pointLightPosition");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformPointLightPosition < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformPointLightRangeInverse = glGetUniformLocation(_glData.shaderProgram, "u_pointLightRangeInverse");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformPointLightRangeInverse < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformSpotLightColor = glGetUniformLocation(_glData.shaderProgram, "u_spotLightColor");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightColor < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformSpotLightPosition = glGetUniformLocation(_glData.shaderProgram, "u_spotLightPosition");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightPosition < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformSpotLightRangeInverse = glGetUniformLocation(_glData.shaderProgram, "u_spotLightRangeInverse");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightRangeInverse < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformSpotLightDirection = glGetUniformLocation(_glData.shaderProgram, "u_spotLightDirection");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightDirection < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformSpotLightInnerAngleCos = glGetUniformLocation(_glData.shaderProgram, "u_spotLightInnerAngleCos");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightInnerAngleCos < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformSpotLightOuterAngleCos = glGetUniformLocation(_glData.shaderProgram, "u_spotLightOuterAngleCos");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightOuterAngleCos < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glData.uniformNormalMatrix = glGetUniformLocation(_glData.shaderProgram, "u_normalMatrix");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformNormalMatrix < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	if (_isC3b)
	{
		_glData.uniformCameraPosition = glGetUniformLocation(_glData.shaderProgram, "u_cameraPosition");
		if (glGetError() != GL_NO_ERROR)
		{
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
			return false;
		}

		if (_glData.uniformCameraPosition < 0)
		{
			Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
			return false;
		}

		_glData.uniformMaterialAmbient = glGetUniformLocation(_glData.shaderProgram, "u_materialAmbient");
		if (glGetError() != GL_NO_ERROR)
		{
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
			return false;
		}

		if (_glData.uniformMaterialAmbient < 0)
		{
			Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
			return false;
		}

		_glData.uniformMaterialDiffuse = glGetUniformLocation(_glData.shaderProgram, "u_materialDiffuse");
		if (glGetError() != GL_NO_ERROR)
		{
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
			return false;
		}

		if (_glData.uniformMaterialDiffuse < 0)
		{
			Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
			return false;
		}

		_glData.uniformMaterialSpecular = glGetUniformLocation(_glData.shaderProgram, "u_materialSpecular");
		if (glGetError() != GL_NO_ERROR)
		{
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
			return false;
		}

		if (_glData.uniformMaterialSpecular < 0)
		{
			Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
			return false;
		}

		_glData.uniformMaterialShininess = glGetUniformLocation(_glData.shaderProgram, "u_materialShininess");
		if (glGetError() != GL_NO_ERROR)
		{
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
			return false;
		}

		if (_glData.uniformMaterialShininess < 0)
		{
			Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
			return false;
		}

		//_glData.uniformMaterialEmissive = glGetUniformLocation(_glData.shaderProgram, "u_materialEmissive");
		//if (glGetError() != GL_NO_ERROR)
		//{
		//	return false;
		//}

		//if (_glData.uniformMaterialEmissive < 0)
		//{
		//	return false;
		//}

		//_glData.uniformMaterialOpacity = glGetUniformLocation(_glData.shaderProgram, "u_materialOpacity");
		//if (glGetError() != GL_NO_ERROR)
		//{
		//	return false;
		//}

		//if (_glData.uniformMaterialOpacity < 0)
		//{
		//	return false;
		//}

		_uniformLightViewMatrix = glGetUniformLocation(_glData.shaderProgram, "u_lightViewMatrix");
		if (glGetError() != GL_NO_ERROR)
		{
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
			return false;
		}

		if (_uniformLightViewMatrix < 0)
		{
			Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
			return false;
		}
	}


	_glDataForShadowMap = createOpenGLProgram(
		// vertex shader
		// ModelData�����g��Ȃ��ꍇ
		//"attribute vec4 a_position;"
		//"uniform mat4 u_lightViewMatrix;" // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��r���[�s��
		//"uniform mat4 u_viewMatrix;"
		//"uniform mat4 u_projectionMatrix;"
		//"void main()"
		//"{"
		//"	gl_Position = u_projectionMatrix * u_lightViewMatrix * u_modelMatrix * getPosition();"
		//"}"
		//// �A�j���[�V�������g���ꍇ
		"attribute vec3 a_position;" // ���ꂪvec3�ɂȂ��Ă���̂ɒ��� TODO:�Ȃ��Ȃ̂��H
		"attribute vec4 a_blendWeight;"
		"attribute vec4 a_blendIndex;"
		""
		"const int SKINNING_JOINT_COUNT = 60;" // TODO:�Ȃ�60�܂łȂ̂��H
		""
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_lightViewMatrix;" // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��r���[�s��
		"uniform mat4 u_projectionMatrix;"
		"uniform mat4 u_matrixPalette[SKINNING_JOINT_COUNT];"
		""
		"varying vec2 v_texCoord;"
		""
		"vec4 getPosition()"
		"{"
		"	mat4 skinMatrix = u_matrixPalette[int(a_blendIndex[0])] * a_blendWeight[0];"
		""
		"	if (a_blendWeight[1] > 0.0)"
		"	{"
		"		skinMatrix += u_matrixPalette[int(a_blendIndex[1])] * a_blendWeight[1];"
		""
		"		if (a_blendWeight[2] > 0.0)"
		"		{"
		"			skinMatrix += u_matrixPalette[int(a_blendIndex[2])] * a_blendWeight[2];"
		""
		"			if (a_blendWeight[3] > 0.0)"
		"			{"
		"				skinMatrix += u_matrixPalette[int(a_blendIndex[3])] * a_blendWeight[3];"
		"			}"
		"		}"
		"	}"
		""
		"	vec4 position = vec4(a_position, 1.0);"
		"	vec4 skinnedPosition = skinMatrix * position;"
		"	skinnedPosition.w = 1.0;"
		"	return skinnedPosition;"
		"}"
		""
		"void main()"
		"{"
		"	gl_Position = u_projectionMatrix * u_lightViewMatrix * u_modelMatrix * getPosition();"
		"}"
		,
		// fragment shader
		"void main()"
		"{"
		"	gl_FragColor = vec4(gl_FragCoord.z);"
		"}"
	);

	_glDataForShadowMap.uniformViewMatrix = glGetUniformLocation(_glDataForShadowMap.shaderProgram, "u_lightViewMatrix");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glDataForShadowMap.uniformViewMatrix < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	_glDataForShadowMap.uniformSkinMatrixPalette = glGetUniformLocation(_glDataForShadowMap.shaderProgram, "u_matrixPalette");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		return false;
	}

	if (_glDataForShadowMap.uniformSkinMatrixPalette < 0)
	{
		Logger::logAssert(false, "�V�F�[�_����ϐ��m�ێ��s�B");
		return false;
	}

	return true;
}

void Sprite3D::setTexture(const std::string& filePath)
{
	// Texture�����[�h���Apng��jpeg�𐶃f�[�^�ɂ��AOpenGL�ɂ�����d�g�݂����˂΁B�BSprite�̃\�[�X���������Ƃ����B
	Image image; // Image��CPU���̃��������g���Ă���̂ł��̃X�R�[�v�ŉ������Ă��悢���̂�����X�^�b�N�Ɏ��
	image.initWithFilePath(filePath);

	_texture = new Texture(); // Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
	_texture->initWithImage(image);
}

void Sprite3D::startAnimation(const std::string& animationName, bool loop /* = false*/)
{
	_elapsedTime = 0.0f;
	_currentAnimation = _animationDatas->animations[animationName];
	_loopAnimation = loop;
}

void Sprite3D::stopAnimation()
{
	_currentAnimation = nullptr;
}

C3bLoader::NodeData* Sprite3D::findJointByName(const std::string& jointName, const std::vector<C3bLoader::NodeData*> children)
{
	for (C3bLoader::NodeData* child : children)
	{
		if (child->id == jointName)
		{
			// ���������炻���Ԃ��B���[�v���I���B
			return child;
		}
		else if (child->children.size() > 0)
		{
			// �����炸�A�q������Ȃ�q��T���ɍs��
			C3bLoader::NodeData* findResult = findJointByName(jointName, child->children);
			if (findResult != nullptr)
			{
				// ���������炻���Ԃ��B���[�v���I���B
				return findResult;
			}
		}
	}

	return nullptr;
}

void Sprite3D::update(float dt)
{
	Node::update(dt);

	if (!_isC3b) {
		return;
	}

	float t = 0; // 0<=t<=1�̃A�j���[�V�����⊮�p�����[�^
	if (_currentAnimation != nullptr)
	{
		_elapsedTime += dt;
		if (_loopAnimation && _elapsedTime > _currentAnimation->totalTime)
		{
			_elapsedTime = 0.0f;
		}

		t = _elapsedTime / _currentAnimation->totalTime;
	}

	_matrixPalette.clear();

	// �A�j���[�V�������s���BAnimate3D::update���Q�l�� C3bLoader::AnimationData�̎g�����Ȃ����̂�
	size_t numSkinJoint = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones.size();
	Logger::logAssert(numSkinJoint == _nodeDatas->nodes[0]->modelNodeDatas[0]->invBindPose.size(), "�W���C���g���͈�v����͂�");

	// ��Ɋe�W���C���g�̃A�j���[�V�����s����쐬����
	for (int i = 0; i < numSkinJoint; ++i)
	{
		const std::string& jointName = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones[i];//TODO: nodes���ɗv�f�͈�Aparts���ɂ�������ł��邱�Ƃ�O��ɂ��Ă���

		// �L�[�t���[����񂪂Ȃ�������ANodeDatas::skeletons::transform�̕��̃c���[����ċA�I�ɃW���C���g�ʒu���W�ϊ��z����v�Z����
		// �܂��͑Ή�����{�[����T���A�ݒ肳��Ă���s����擾����
		C3bLoader::NodeData* node = findJointByName(jointName, _nodeDatas->skeleton);
		Logger::logAssert(node != nullptr, "nodes�̕��ɂ��������̂�skeletons����{�[�����Ō����������Ȃ��B");

		// ��U�A�A�j���[�V�����g��Ȃ����ɏ�����
		node->animatedTransform = Mat4::ZERO;

		if (_currentAnimation != nullptr && _currentAnimation->translationKeyFrames.find(jointName) != _currentAnimation->translationKeyFrames.end())
		{
			// �A�j���[�V�������ŁA�f�[�^�ɂ��̃{�[���̃L�[�t���[����񂪂������Ƃ��͂����炩�������s��ŏ㏑������
			const Vec3& translation = _currentAnimation->evaluateTranslation(jointName, t);
			const Quaternion& rotation = _currentAnimation->evaluateRotation(jointName, t);
			const Vec3& scale = _currentAnimation->evaluateScale(jointName, t);

			node->animatedTransform = Mat4::createTransform(translation, rotation, scale);
		}
	}

	// ���ɃW���C���g�̃}�g���b�N�X�p���b�g�����ׂċ��߂�
	for (int i = 0; i < numSkinJoint; ++i)
	{
		const std::string& jointName = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones[i];//TODO: nodes���ɗv�f�͈�Aparts���ɂ�������ł��邱�Ƃ�O��ɂ��Ă���

		// �L�[�t���[����񂪂Ȃ�������ANodeDatas::skeletons::transform�̕��̃c���[����ċA�I�ɃW���C���g�ʒu���W�ϊ��z����v�Z����
		// �܂��͑Ή�����{�[����T���A�ݒ肳��Ă���s����擾����
		C3bLoader::NodeData* node = findJointByName(jointName, _nodeDatas->skeleton);
		if (node == nullptr)
		{
			Logger::log("nodes�̕��ɂ��������̂�skeletons����{�[�����Ō����������Ȃ��B");
		}
		Logger::logAssert(node != nullptr, "c3b�̎d�l��A�W���C���g����������Ȃ��͂����Ȃ��B");

		Mat4 transform = (node->animatedTransform != Mat4::ZERO) ? node->animatedTransform : node->transform;

		// �e�����[�g�܂ł����̂ڂ��ă��[���h�s����v�Z����
		while (node->parent != nullptr)
		{
			// TODO:�e�̃{�[�����A�j���[�V�������Ă�\�����������߂ɁANodeData�ɂ������Matrix��������Ƃ��������ȕ��@�B�{����Skeleton3D��Bone3D�݂����ɕʂ̊K�w�\���f�[�^��ێ�������������
			const Mat4& parentTransform = (node->parent->animatedTransform != Mat4::ZERO) ? node->parent->animatedTransform : node->parent->transform;
			transform = parentTransform * transform;
			node = node->parent;
		}

		// �{�[���̔z�u�s��
		const Mat4& invBindPose = _nodeDatas->nodes[0]->modelNodeDatas[0]->invBindPose[i];//TODO: nodes���ɗv�f�͈�Aparts���ɂ�������ł��邱�Ƃ�O��ɂ��Ă���

		Mat4 matrix = transform * invBindPose;
		_matrixPalette.push_back(matrix);
	}
}

void Sprite3D::renderShadowMap()
{
	Node::renderShadowMap();

	bool makeShadowMap = false;
	DirectionalLight::ShadowMapData shadowMapData;

	for (Light* light : Director::getLight())
	{
		switch (light->getLightType())
		{
		case LightType::AMBIENT:
			break;
		case LightType::DIRECTION: {
			DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
			// TODO:�Ƃ肠�����e����DirectionalLight�݂̂�z��
			// ���̕����Ɍ����ăV���h�E�}�b�v�����J�����������Ă���ƍl���A�J�������猩�����f�����W�n�ɂ���
			if (dirLight->hasShadowMap())
			{
				makeShadowMap = true;
				shadowMapData = dirLight->getShadowMapData();
			}
		}
			break;
		case LightType::POINT: {
		}
			break;
		case LightType::SPOT: {
		}
		default:
			break;
		}
	}

	if (!makeShadowMap)
	{
		// �V���h�E�}�b�v��K�v�Ƃ��郉�C�g���Ȃ���Ή������Ȃ�
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapData.frameBufferId);

	//TODO:�V���h�E�}�b�v�̑傫���͉�ʃT�C�Y�Ɠ����ɂ��Ă���
	glViewport(0, 0, Director::getInstance()->getWindowSize().width, Director::getInstance()->getWindowSize().height);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(_glDataForShadowMap.shaderProgram);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glUniformMatrix4fv(
		_glDataForShadowMap.uniformViewMatrix,
		1,
		GL_FALSE,
		(GLfloat*)shadowMapData.viewMatrix.m
	);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	// TODO:Vec3��Mat4�ɓ��ɂ���-���Z�q���Ȃ���

	// �s��̐ݒ�
	glUniformMatrix4fv(_glDataForShadowMap.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glDataForShadowMap.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	// ���_�����̐ݒ�
	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)AttributeLocation::BLEND_WEIGHT);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)AttributeLocation::BLEND_INDEX);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
	
	Logger::logAssert(_isC3b, "�V���h�E�}�b�v�g�p����Ƃ���obj���Ή�");
	C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
	for (int i = 0, offset = 0; i < meshData->numAttribute; ++i)
	{
		const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
		glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		offset += attrib.size;
	}

	Logger::logAssert(_matrixPalette.size() > 0, "�}�g���b�N�X�p���b�g��0�łȂ��O��");
	glUniformMatrix4fv(_glDataForShadowMap.uniformSkinMatrixPalette, _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette[0].m));
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, &_indices[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // �f�t�H���g�t���[���o�b�t�@�ɖ߂�
}

void Sprite3D::renderWithShadowMap()
{
	Node::renderWithShadowMap();

	//glViewport(0, 0, Director::getInstance()->getWindowSize().width, Director::getInstance()->getWindowSize().height);

	// cocos2d-x��TriangleCommand���s���Ă�`������ȁB�B�e�N�X�`���o�C���h��Texture2D�ł���Ă�̂ɑ��v���H
	glUseProgram(_glData.shaderProgram);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glUniform3f(_glData.uniformMultipleColor, getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	// �s��̐ݒ�
	glUniformMatrix4fv(_glData.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	const Mat4& normalMatrix = calculateNormalMatrix(getModelMatrix());
	glUniformMatrix4fv(_glData.uniformNormalMatrix, 1, GL_FALSE, (GLfloat*)&normalMatrix.m);

	// ���C�g�̐ݒ�
	// TODO:����A���C�g�͊e��ނ��ƂɈ�������������ĂȂ��B�Ō�̂�ŏ㏑���B
	for (Light* light : Director::getLight())
	{
		const Color3B& lightColor = light->getColor();
		float intensity = light->getIntensity();

		switch (light->getLightType())
		{
		case LightType::AMBIENT:
			glUniform3f(_glData.uniformAmbientLightColor, lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
			break;
		case LightType::DIRECTION: {
			glUniform3f(_glData.uniformDirectionalLightColor, lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

			DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
			Vec3 direction = dirLight->getDirection();
			direction.normalize();
			glUniform3fv(_glData.uniformDirectionalLightDirection, 1, (GLfloat*)&direction);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

			// TODO:�Ƃ肠�����e����DirectionalLight�݂̂�z��
			// ���̕����Ɍ����ăV���h�E�}�b�v�����J�����������Ă���ƍl���A�J�������猩�����f�����W�n�ɂ���
			if (dirLight->hasShadowMap())
			{
				glUniformMatrix4fv(
					_uniformLightViewMatrix,
					1,
					GL_FALSE,
					(GLfloat*)dirLight->getShadowMapData().viewMatrix.m
				);
				// TODO:Vec3��Mat4�ɓ��ɂ���-���Z�q���Ȃ���

				if (_isC3b) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, dirLight->getShadowMapData().textureId);
					glUniform1i(_uniformShadowTexture, 1);
					glActiveTexture(GL_TEXTURE0);
				}
			}
		}
			break;
		case LightType::POINT: {
			glUniform3f(_glData.uniformPointLightColor, lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

			glUniform3fv(_glData.uniformPointLightPosition, 1, (GLfloat*)&light->getPosition()); // ���C�g�ɂ��Ă̓��[�J�����W�łȂ����[���h���W�ł���O��
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

			PointLight* pointLight = static_cast<PointLight*>(light);
			glUniform1f(_glData.uniformPointLightRangeInverse, 1.0f / pointLight->getRange());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		}
			break;
		case LightType::SPOT: {
			glUniform3f(_glData.uniformSpotLightColor, lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

			glUniform3fv(_glData.uniformSpotLightPosition, 1, (GLfloat*)&light->getPosition());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

			SpotLight* spotLight = static_cast<SpotLight*>(light);
			Vec3 direction = spotLight->getDirection();
			direction.normalize();
			glUniform3fv(_glData.uniformSpotLightDirection, 1, (GLfloat*)&direction);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

			glUniform1f(_glData.uniformSpotLightRangeInverse, 1.0f / spotLight->getRange());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

			glUniform1f(_glData.uniformSpotLightInnerAngleCos, spotLight->getInnerAngleCos());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

			glUniform1f(_glData.uniformSpotLightOuterAngleCos, spotLight->getOuterAngleCos());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		}
		default:
			break;
		}
	}

	// ���_�����̐ݒ�
	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)AttributeLocation::NORMAL);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)AttributeLocation::BLEND_WEIGHT);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)AttributeLocation::BLEND_INDEX);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glEnableVertexAttribArray(_glData.attributeTextureCoordinates);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	if (_isObj)
	{
		glVertexAttribPointer((GLuint)AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].position);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		glVertexAttribPointer((GLuint)AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].normal);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		glVertexAttribPointer(_glData.attributeTextureCoordinates, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	}
	else if (_isC3b)
	{
		// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
		C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
		for (int i = 0, offset = 0; i < meshData->numAttribute; ++i)
		{
			const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
			glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
			offset += attrib.size;
		}
	}

	// �X�L�j���O�̃}�g���b�N�X�p���b�g�̐ݒ�
	if (_isC3b) {
		Logger::logAssert(_matrixPalette.size() > 0, "�}�g���b�N�X�p���b�g��0�łȂ��O��");
		glUniformMatrix4fv(_glData.uniformSkinMatrixPalette, _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette[0].m));
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	}

	// TODO:monguri:����
	if (_isC3b) {
		glUniform3fv(_glData.uniformCameraPosition, 1, (GLfloat*)&Director::getCamera().getPosition());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniform3fv(_glData.uniformMaterialAmbient, 1, (GLfloat*)&_ambient);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniform3fv(_glData.uniformMaterialDiffuse, 1, (GLfloat*)&_diffuse);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniform3fv(_glData.uniformMaterialSpecular, 1, (GLfloat*)&_specular);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniform1f(_glData.uniformMaterialShininess, _shininess);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		//glUniform3fv(_glData.uniformMaterialEmissive, 1, (GLfloat*)&_emissive);
		//Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		//glUniform1f(_glData.uniformMaterialOpacity, 1, (GLfloat*)&_emissive);
		//Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	}

	glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, &_indices[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	glBindTexture(GL_TEXTURE_2D, 0);
}

Mat4 Sprite3D::calculateNormalMatrix(const Mat4& modelMatrix)
{
	Mat4 normalMatrix = modelMatrix;
	normalMatrix.m[3][0] = 0.0f;
	normalMatrix.m[3][1] = 0.0f;
	normalMatrix.m[3][2] = 0.0f;
	normalMatrix.m[3][3] = 1.0f;
	normalMatrix.inverse();
	normalMatrix.transpose();
	return normalMatrix;
}

} // namespace mgrrenderer
