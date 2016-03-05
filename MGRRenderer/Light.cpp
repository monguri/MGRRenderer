#include "Light.h"
#include "Director.h"

namespace mgrrenderer
{

Light::Light() : _intensity(1.0f)
{
}

AmbientLight::AmbientLight(const Color3B& color)
{
	setColor(color);
}

DirectionalLight::DirectionalLight(const Vec3& direction, const Color3B& color) : _direction(direction), _hasShadowMap(false)
{
	setColor(color);
}

void DirectionalLight::prepareShadowMap(const Vec3& targetPosition, float cameraDistanceFromTaret, const Size& size)
{
	_hasShadowMap = true;

	_shadowMapData.viewMatrix = Mat4::createLookAt(
		getDirection() * (-1) * cameraDistanceFromTaret + targetPosition, // 近すぎるとカメラに入らないので適度に離す
		targetPosition,
		Vec3(0.0f, 1.0f, 0.0f) // とりあえずy方向を上にして固定
	);

	//_shadowMapData.projectionMatrix = Mat4::createPerspective(60.0, size.width / size.height, 10, cameraDistanceFromTaret + size.height / 2.0f);
	_shadowMapData.projectionMatrix = Mat4::createPerspective(60.0, size.width / size.height, 10, 10000.0f);

	// デプステクスチャ作成
	glGenTextures(1, &_shadowMapData.textureId);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	Logger::logAssert(_shadowMapData.textureId != 0, "デプステクスチャ生成失敗");

	glBindTexture(GL_TEXTURE_2D, _shadowMapData.textureId);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat boarderColor[] = {1.0f, 0.0f, 0.0f, 0.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, boarderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	//TODO: 適当にデプステクスチャ解像度はウインドウサイズと同じにしておく
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.width, size.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glBindTexture(GL_TEXTURE_2D, 0);

	// デプステクスチャに描画するためのフレームバッファ作成
	glGenFramebuffers(1, &_shadowMapData.frameBufferId);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	Logger::logAssert(_shadowMapData.frameBufferId != 0, "フレームバッファ生成失敗");

	glBindFramebuffer(GL_FRAMEBUFFER, _shadowMapData.frameBufferId);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowMapData.textureId, 0);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	GLenum drawBuffers[] = {GL_NONE};
	glDrawBuffers(1, drawBuffers);

	Logger::logAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "デプスシャドウ用のフレームバッファが完成してない");

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // デフォルトのフレームバッファに戻す
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
}

void DirectionalLight::beginRenderShadowMap()
{
	Logger::logAssert(hasShadowMap(), "beginRenderShadowMap呼び出しはシャドウマップを使う前提");

	_beginRenderCommand.init([=]
	{
		glBindFramebuffer(GL_FRAMEBUFFER, getShadowMapData().frameBufferId);

		glClear(GL_DEPTH_BUFFER_BIT);

		//TODO:シャドウマップの大きさは画面サイズと同じにしている
		glViewport(0, 0, Director::getInstance()->getWindowSize().width, Director::getInstance()->getWindowSize().height);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	});

	Director::getRenderer().addCommand(&_beginRenderCommand);
}

void DirectionalLight::endRenderShadowMap()
{
	Logger::logAssert(hasShadowMap(), "endRenderShadowMap呼び出しはシャドウマップを使う前提");

	_endRenderCommand.init([=]
	{
		glDisable(GL_CULL_FACE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // デフォルトフレームバッファに戻す
	});

	Director::getRenderer().addCommand(&_endRenderCommand);
}

PointLight::PointLight(const Vec3& position, const Color3B& color, float range) : _range(range)
{
	setPosition(position);
	setColor(color);
}

SpotLight::SpotLight(const Vec3& position, const Vec3& direction, const Color3B& color, float range, float innerAngle, float outerAngle) :
_direction(direction),
_range(range),
_innerAngleCos(cosf(innerAngle)),
_outerAngleCos(cosf(outerAngle))
{
	setPosition(position);
	setColor(color);
}

} // namespace mgrrenderer
