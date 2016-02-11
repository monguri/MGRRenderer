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

	_glData = createOpenGLProgram(
		// vertex shader
		"attribute vec4 a_position;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_viewMatrix;"
		"uniform mat4 u_lightViewMatrix;" // 影付けに使うライトをカメラに見立てたビュー行列
		"uniform mat4 u_projectionMatrix;"
		"varying vec4 v_lightPosition;"
		"void main()"
		"{"
		"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * a_position;"
		"	v_lightPosition = u_projectionMatrix * u_lightViewMatrix * u_modelMatrix * a_position;"
		"}"
		,
		// fragment shader
		"uniform sampler2D u_shadowTexture;"
		"uniform vec3 u_multipleColor;"
		"varying vec4 v_lightPosition;"
		"void main()"
		"{"
		"	gl_FragColor = vec4(u_multipleColor, 1.0);"
		""
		"	vec4 depthCheck = v_lightPosition / v_lightPosition.w;"
		"	depthCheck = depthCheck / 2.0 + 0.5;"
		"	float textureDepth = texture2D(u_shadowTexture, depthCheck.xy).z;"
		"	if (depthCheck.z > textureDepth + 0.0003)" // TODO:後で修正
		"	{"
		"		gl_FragColor.rgb *= 0.5;" // TODO:これも定数かけるなんて中途半端。後で修正。僕は0.5にしている。
		"	}"
		"}"
		);

	// TODO:シャドウマップやるかの判定を入れる
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

	return true;
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

	for (Light* light : Director::getLight())
	{
		const Color3B& lightColor = light->getColor();
		float intensity = light->getIntensity();

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
				glUniformMatrix4fv(
					_uniformLightViewMatrix,
					1,
					GL_FALSE,
					(GLfloat*)dirLight->getShadowMapData().viewMatrix.m
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
		}
			break;
		case LightType::SPOT: {
		}
		default:
			break;
		}
	}

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glLineWidth(1.0f);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
}

} // namespace mgrrenderer
