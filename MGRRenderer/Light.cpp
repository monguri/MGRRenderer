#include "Light.h"
#include "Director.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DTexture.h"
#endif

namespace mgrrenderer
{

Light::Light() : _intensity(1.0f)
{
}

AmbientLight::AmbientLight(const Color3B& color)
{
	setColor(color);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color4F(color) * getIntensity();
#endif
}

void AmbientLight::setColor(const Color3B& color)
{
	Light::setColor(color);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color4F(color) * getIntensity();
#endif
}

void AmbientLight::setIntensity(float intensity)
{
	Light::setIntensity(intensity);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color4F(getColor()) * intensity;
#endif
}

DirectionalLight::DirectionalLight(const Vec3& direction, const Color3B& color) :
#if defined(MGRRENDERER_USE_OPENGL)
	_hasShadowMap(false),
#endif
	_direction(direction)
{
	setColor(color);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_shadowMapData.depthTexture = nullptr;
	
	Vec3 directionVec = direction;
	directionVec.normalize();
	_constantBufferData.direction = Mat4::CHIRARITY_CONVERTER * directionVec;
	_constantBufferData.hasShadowMap = 0.0f;
	_constantBufferData.color = Color4F(color) * getIntensity();

#elif defined(MGRRENDERER_USE_OPENGL)
	_shadowMapData.frameBufferId = 0;
	_shadowMapData.textureId = 0;
#endif
}

DirectionalLight::~DirectionalLight()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	if (_shadowMapData.depthTexture != nullptr)
	{
		delete _shadowMapData.depthTexture;
		_shadowMapData.depthTexture = nullptr;
	}
#elif defined(MGRRENDERER_USE_OPENGL)
	if (_shadowMapData.frameBufferId != 0)
	{
		glDeleteFramebuffers(1, &_shadowMapData.frameBufferId);
		_shadowMapData.frameBufferId = 0;
	}

	if (_shadowMapData.textureId != 0)
	{
		glDeleteTextures(1, &_shadowMapData.textureId);
		_shadowMapData.textureId = 0;
	}
#endif
}

void DirectionalLight::setColor(const Color3B& color)
{
	Light::setColor(color);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color4F(color) * getIntensity();
#endif
}

void DirectionalLight::setIntensity(float intensity)
{
	Light::setIntensity(intensity);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color4F(getColor()) * intensity;
#endif
}

void DirectionalLight::prepareShadowMap(const Vec3& targetPosition, float cameraDistanceFromTaret, const Size& size)
{
	_shadowMapData.viewMatrix = Mat4::createLookAt(
		getDirection() * (-1) * cameraDistanceFromTaret + targetPosition, // 近すぎるとカメラに入らないので適度に離す
		targetPosition,
		Vec3(0.0f, 1.0f, 0.0f) // とりあえずy方向を上にして固定
	);

	//_shadowMapData.projectionMatrix = Mat4::createPerspective(60.0, size.width / size.height, 10, cameraDistanceFromTaret + size.height / 2.0f);
	_shadowMapData.projectionMatrix = Mat4::createPerspective(60.0, size.width / size.height, 10.0f, 10000.0f);

	// デプステクスチャ作成
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.hasShadowMap = 1.0f;

	ID3D11Device* device = Director::getInstance()->getDirect3dDevice();

	_shadowMapData.depthTexture = new D3DTexture();
	_shadowMapData.depthTexture->initDepthStencilTexture(size);
#elif defined(MGRRENDERER_USE_OPENGL)
	_hasShadowMap = true;

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
#endif
}

bool DirectionalLight::hasShadowMap() const
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	return _constantBufferData.hasShadowMap > 0.0f;
#elif defined(MGRRENDERER_USE_OPENGL)
	return _hasShadowMap;
#endif
}

void DirectionalLight::prepareShadowMapRendering()
{
	Logger::logAssert(hasShadowMap(), "beginRenderShadowMap呼び出しはシャドウマップを使う前提");

	_prepareShadowMapRenderingCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
		direct3dContext->ClearState();
		direct3dContext->ClearDepthStencilView(_shadowMapData.depthTexture->getDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		D3D11_VIEWPORT viewport[1];
		viewport[0].TopLeftX = 0.0f;
		viewport[0].TopLeftY = 0.0f;
		viewport[0].Width = Director::getInstance()->getWindowSize().width;
		viewport[0].Height = Director::getInstance()->getWindowSize().height;
		viewport[0].MinDepth = 0.0f;
		viewport[0].MaxDepth = 1.0f;
		direct3dContext->RSSetViewports(1, viewport);
		
		ID3D11RenderTargetView* renderTarget[1] = {nullptr}; // シャドウマップ描画はDepthStencilViewはあるがRenderTargetはないのでnullでいい
		direct3dContext->OMSetRenderTargets(1, renderTarget, _shadowMapData.depthTexture->getDepthStencilView());
#elif defined(MGRRENDERER_USE_OPENGL)
		glBindFramebuffer(GL_FRAMEBUFFER, getShadowMapData().frameBufferId);

		glClear(GL_DEPTH_BUFFER_BIT);

		//TODO:シャドウマップの大きさは画面サイズと同じにしている
		glViewport(0, 0, Director::getInstance()->getWindowSize().width, Director::getInstance()->getWindowSize().height);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
#endif
	});

	Director::getRenderer().addCommand(&_prepareShadowMapRenderingCommand);
}

PointLight::PointLight(const Vec3& position, const Color3B& color, float range) : _range(range)
{
	setPosition(position);
	setColor(color);

#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color4F(color) * getIntensity();
	_constantBufferData.position = position;
	_constantBufferData.rangeInverse = 1.0f / range;
#endif
}

void PointLight::setColor(const Color3B& color)
{
	Light::setColor(color);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color4F(color) * getIntensity();
#endif
}

void PointLight::setIntensity(float intensity)
{
	Light::setIntensity(intensity);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color4F(getColor()) * intensity;
#endif
}

SpotLight::SpotLight(const Vec3& position, const Vec3& direction, const Color3B& color, float range, float innerAngle, float outerAngle) :
_direction(direction),
_range(range),
_innerAngleCos(cosf(innerAngle)),
_outerAngleCos(cosf(outerAngle))
{
	setPosition(position);
	setColor(color);

#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.position = position;
	_constantBufferData.rangeInverse = 1.0f / range;
	_constantBufferData.color = Color3F(color) * getIntensity();
	_constantBufferData.innerAngleCos = cosf(innerAngle);
	_constantBufferData.direction = Mat4::CHIRARITY_CONVERTER * direction;
	_constantBufferData.outerAngleCos = cosf(outerAngle);
#endif
}

void SpotLight::setColor(const Color3B& color)
{
	Light::setColor(color);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color3F(color) * getIntensity();
#endif
}

void SpotLight::setIntensity(float intensity)
{
	Light::setIntensity(intensity);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color3F(getColor()) * intensity;
#endif
}

} // namespace mgrrenderer
