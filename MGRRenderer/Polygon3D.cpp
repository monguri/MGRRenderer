#include "Polygon3D.h"
#include "Director.h"
#include "Camera.h"
#include "Light.h"

namespace mgrrenderer
{

Polygon3D::~Polygon3D()
{
	destroyOpenGLProgram(_glData);
}

bool Polygon3D::initWithVertexArray(const std::vector<Vec3>& vertexArray)
{
	if (vertexArray.size() % 3 != 0)
	{
		// 三角形の頂点のため頂点数が3の倍数であることを前提にする
		return false;
	}

	_vertexArray = vertexArray;

	_normalArray.clear();

	// TODO:GL_TRIANGLESなので重複した法線の場合はパイプラインでどっちが後に処理されたかによる。そこは考慮していない。考慮する場合はGL_TRIANGLESをそもそも使わない
	size_t numPolygon = vertexArray.size() / 3;
	for (int i = 0; i < numPolygon; ++i)
	{
		// 0,1,2と左回りになっているのを前提とする
		Vec3 normal0 = Vec3::cross(vertexArray[3 * i + 1] - vertexArray[3 * i], vertexArray[3 * i + 2] - vertexArray[3 * i]); // 左回りを前提とする
		normal0.normalize();
		_normalArray.push_back(normal0);

		Vec3 normal1 = Vec3::cross(vertexArray[3 * i + 2] - vertexArray[3 * i + 1], vertexArray[3 * i] - vertexArray[3 * i + 1]);
		normal1.normalize();
		_normalArray.push_back(normal1);

		Vec3 normal2 = Vec3::cross(vertexArray[3 * i] - vertexArray[3 * i + 2], vertexArray[3 * i + 1] - vertexArray[3 * i + 2]);
		normal2.normalize();
		_normalArray.push_back(normal2);
	}

	// TODO:objのシェーダとほぼ同じ。共通化したい。
	_glData = createOpenGLProgram(
		// vertex shader
		"attribute vec4 a_position;"
		"attribute vec4 a_normal;"
		"varying vec4 v_normal;"
		"varying vec4 v_lightPosition;"
		"varying vec3 v_vertexToPointLightDirection;"
		"varying vec3 v_vertexToSpotLightDirection;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_viewMatrix;"
		"uniform mat4 u_lightViewMatrix;" // 影付けに使うライトをカメラに見立てたビュー行列
		"uniform mat4 u_projectionMatrix;"
		"uniform mat4 u_depthBiasMatrix;"
		"uniform mat4 u_normalMatrix;"
		"uniform vec3 u_pointLightPosition;"
		"uniform vec3 u_spotLightPosition;"
		"void main()"
		"{"
		"	vec4 worldPosition = u_modelMatrix * a_position;"
		"	v_vertexToPointLightDirection = u_pointLightPosition - worldPosition.xyz;"
		"	v_vertexToSpotLightDirection = u_spotLightPosition - worldPosition.xyz;"
		"	gl_Position = u_projectionMatrix * u_viewMatrix * worldPosition;"
		"	v_normal = u_normalMatrix * a_normal;" // scale変換に対応するためにモデル行列の逆行列を転置したものを用いる
		"	v_lightPosition = u_depthBiasMatrix * u_projectionMatrix * u_lightViewMatrix * worldPosition;"
		"}"
		,
		// fragment shader
		"uniform sampler2DShadow u_shadowTexture;"
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
		"varying vec3 v_vertexToPointLightDirection;"
		"varying vec3 v_vertexToSpotLightDirection;"
		"varying vec4 v_lightPosition;"
		"vec3 computeLightedColor(vec3 normalVector, vec3 lightDirection, vec3 lightColor, float attenuation)"
		"{"
		"	float diffuse = max(dot(normalVector, lightDirection), 0.0);"
		"	vec3 diffuseColor = lightColor * diffuse * attenuation;"
		"	return diffuseColor;"
		"}"
		""
		"void main()"
		"{"
		"	vec4 ambientLightColor = vec4(u_ambientLightColor, 1.0);"
		""
		"	vec3 normal = normalize(v_normal.xyz);" // データ形式の時点でnormalizeされてない法線がある模様
		"	vec4 diffuseSpecularLightColor = vec4(0.0, 0.0, 0.0, 1.0);"
		"	diffuseSpecularLightColor.rgb += computeLightedColor(normal, -u_directionalLightDirection, u_directionalLightColor, 1.0);"
		""
		"	vec3 dir = v_vertexToPointLightDirection * u_pointLightRangeInverse;"
		"	float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);"
		"	diffuseSpecularLightColor.rgb += computeLightedColor(normal, normalize(v_vertexToPointLightDirection), u_pointLightColor, attenuation);"
		""
		"	dir = v_vertexToSpotLightDirection * u_spotLightRangeInverse;"
		"	attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);"
		"	vec3 vertexToSpotLightDirection = normalize(v_vertexToSpotLightDirection);"
		"	float spotCurrentAngleCos = dot(u_spotLightDirection, -vertexToSpotLightDirection);"
		"	attenuation *= smoothstep(u_spotLightOuterAngleCos, u_spotLightInnerAngleCos, spotCurrentAngleCos);"
		"	attenuation = clamp(attenuation, 0.0, 1.0);"
		"	diffuseSpecularLightColor.rgb += computeLightedColor(normal, vertexToSpotLightDirection, u_spotLightColor, attenuation);"
		""
		"	float outShadowFlag = textureProj(u_shadowTexture, v_lightPosition);"
		"	gl_FragColor = vec4(u_multipleColor, 1.0) * vec4((diffuseSpecularLightColor.rgb * outShadowFlag + ambientLightColor.rgb), 1.0);" // テクスチャ番号は0のみに対応
		"}"
	);

	_uniformShadowTexture = glGetUniformLocation(_glData.shaderProgram, "u_shadowTexture");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_uniformShadowTexture < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformAmbientLightColor = glGetUniformLocation(_glData.shaderProgram, "u_ambientLightColor");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformAmbientLightColor < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformDirectionalLightColor = glGetUniformLocation(_glData.shaderProgram, "u_directionalLightColor");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformDirectionalLightColor < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformDirectionalLightDirection = glGetUniformLocation(_glData.shaderProgram, "u_directionalLightDirection");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformDirectionalLightDirection < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformPointLightColor = glGetUniformLocation(_glData.shaderProgram, "u_pointLightColor");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformPointLightColor < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformPointLightPosition = glGetUniformLocation(_glData.shaderProgram, "u_pointLightPosition");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformPointLightPosition < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformPointLightRangeInverse = glGetUniformLocation(_glData.shaderProgram, "u_pointLightRangeInverse");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformPointLightRangeInverse < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformSpotLightColor = glGetUniformLocation(_glData.shaderProgram, "u_spotLightColor");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightColor < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformSpotLightPosition = glGetUniformLocation(_glData.shaderProgram, "u_spotLightPosition");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightPosition < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformSpotLightRangeInverse = glGetUniformLocation(_glData.shaderProgram, "u_spotLightRangeInverse");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightRangeInverse < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformSpotLightDirection = glGetUniformLocation(_glData.shaderProgram, "u_spotLightDirection");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightDirection < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformSpotLightInnerAngleCos = glGetUniformLocation(_glData.shaderProgram, "u_spotLightInnerAngleCos");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightInnerAngleCos < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformSpotLightOuterAngleCos = glGetUniformLocation(_glData.shaderProgram, "u_spotLightOuterAngleCos");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformSpotLightOuterAngleCos < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glData.uniformNormalMatrix = glGetUniformLocation(_glData.shaderProgram, "u_normalMatrix");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glData.uniformNormalMatrix < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_uniformDepthBiasMatrix = glGetUniformLocation(_glData.shaderProgram, "u_depthBiasMatrix");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_uniformDepthBiasMatrix < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	// TODO:ライトの判定入れないと
	_uniformLightViewMatrix = glGetUniformLocation(_glData.shaderProgram, "u_lightViewMatrix");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_uniformLightViewMatrix < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}

	_glDataForShadowMap = createOpenGLProgram(
		// vertex shader
		// ModelDataしか使わない場合
		"attribute vec4 a_position;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_lightViewMatrix;" // 影付けに使うライトをカメラに見立てたビュー行列
		"uniform mat4 u_projectionMatrix;"
		"void main()"
		"{"
		"	gl_Position = u_projectionMatrix * u_lightViewMatrix * u_modelMatrix * a_position;"
		"}"
		,
		// fragment shader
		"void main()"
		"{" // 何もせずとも深度は自動で書き込まれる 
		"}"
	);

	_glDataForShadowMap.uniformViewMatrix = glGetUniformLocation(_glDataForShadowMap.shaderProgram, "u_lightViewMatrix");
	if (glGetError() != GL_NO_ERROR)
	{
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		return false;
	}

	if (_glDataForShadowMap.uniformViewMatrix < 0)
	{
		Logger::logAssert(false, "シェーダから変数確保失敗。");
		return false;
	}
	return true;
}

void Polygon3D::renderShadowMap()
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
			// TODO:とりあえず影つけはDirectionalLightのみを想定
			// 光の方向に向けてシャドウマップを作るカメラが向いていると考え、カメラから見たモデル座標系にする
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
		// シャドウマップを必要とするライトがなければ何もしない
		return;
	}

	glUseProgram(_glDataForShadowMap.shaderProgram);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glUniformMatrix4fv(
		_glDataForShadowMap.uniformViewMatrix,
		1,
		GL_FALSE,
		(GLfloat*)shadowMapData.viewMatrix.m
	);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	// TODO:Vec3やMat4に頭につける-演算子作らないと

	glUniformMatrix4fv(_glDataForShadowMap.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glDataForShadowMap.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glEnableVertexAttribArray((GLuint)AttributeLocation::NORMAL);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glVertexAttribPointer((GLuint)AttributeLocation::NORMAL, sizeof(_normalArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_normalArray[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
}

void Polygon3D::renderWithShadowMap()
{
	glEnable(GL_DEPTH_TEST);

	glUseProgram(_glData.shaderProgram);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glUniform3f(_glData.uniformMultipleColor, getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glUniformMatrix4fv(_glData.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	const Mat4& normalMatrix = Mat4::createNormalMatrix(getModelMatrix());
	glUniformMatrix4fv(_glData.uniformNormalMatrix, 1, GL_FALSE, (GLfloat*)&normalMatrix.m);

	// ライトの設定
	// TODO:現状、ライトは各種類ごとに一個ずつしか処理してない。最後のやつで上書き。
	for (Light* light : Director::getLight())
	{
		const Color3B& lightColor = light->getColor();
		float intensity = light->getIntensity();

		switch (light->getLightType())
		{
		case LightType::AMBIENT:
			glUniform3f(_glData.uniformAmbientLightColor, lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
			break;
		case LightType::DIRECTION: {
			glUniform3f(_glData.uniformDirectionalLightColor, lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
			Vec3 direction = dirLight->getDirection();
			direction.normalize();
			glUniform3fv(_glData.uniformDirectionalLightDirection, 1, (GLfloat*)&direction);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			// TODO:とりあえず影つけはDirectionalLightのみを想定
			// 光の方向に向けてシャドウマップを作るカメラが向いていると考え、カメラから見たモデル座標系にする
			if (dirLight->hasShadowMap())
			{
				glUniformMatrix4fv(
					_uniformLightViewMatrix,
					1,
					GL_FALSE,
					(GLfloat*)dirLight->getShadowMapData().viewMatrix.m
				);

				static const Mat4& depthBiasMatrix = Mat4::createScale(Vec3(0.5f, 0.5f, 0.5f)) * Mat4::createTranslation(Vec3(1.0f, 1.0f, 1.0f));

				glUniformMatrix4fv(
					_uniformDepthBiasMatrix,
					1,
					GL_FALSE,
					(GLfloat*)depthBiasMatrix.m
				);
				// TODO:Vec3やMat4に頭につける-演算子作らないと

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, dirLight->getShadowMapData().textureId);
				glUniform1i(_uniformShadowTexture, 1);
				glActiveTexture(GL_TEXTURE0);
			}
		}
			break;
		case LightType::POINT: {
			glUniform3f(_glData.uniformPointLightColor, lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform3fv(_glData.uniformPointLightPosition, 1, (GLfloat*)&light->getPosition()); // ライトについてはローカル座標でなくワールド座標である前提
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			PointLight* pointLight = static_cast<PointLight*>(light);
			glUniform1f(_glData.uniformPointLightRangeInverse, 1.0f / pointLight->getRange());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		}
			break;
		case LightType::SPOT: {
			glUniform3f(_glData.uniformSpotLightColor, lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform3fv(_glData.uniformSpotLightPosition, 1, (GLfloat*)&light->getPosition());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			SpotLight* spotLight = static_cast<SpotLight*>(light);
			Vec3 direction = spotLight->getDirection();
			direction.normalize();
			glUniform3fv(_glData.uniformSpotLightDirection, 1, (GLfloat*)&direction);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform1f(_glData.uniformSpotLightRangeInverse, 1.0f / spotLight->getRange());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform1f(_glData.uniformSpotLightInnerAngleCos, spotLight->getInnerAngleCos());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform1f(_glData.uniformSpotLightOuterAngleCos, spotLight->getOuterAngleCos());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		}
		default:
			break;
		}
	}

	glLineWidth(1.0f);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glEnableVertexAttribArray((GLuint)AttributeLocation::NORMAL);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glVertexAttribPointer((GLuint)AttributeLocation::NORMAL, sizeof(_normalArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_normalArray[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
}

} // namespace mgrrenderer
