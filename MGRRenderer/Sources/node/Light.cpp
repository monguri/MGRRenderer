#include "Light.h"
#include "renderer/Director.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "renderer/D3DTexture.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "renderer/GLFrameBuffer.h"
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
	Vec3 directionVec = direction;
	directionVec.normalize();
	_constantBufferData.direction = directionVec;
	_constantBufferData.color = Color4F(color) * getIntensity();
	_constantBufferData.hasShadowMap = 0.0f;
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
	if (_shadowMapData.depthFrameBuffer != nullptr)
	{
		delete _shadowMapData.depthFrameBuffer;
		_shadowMapData.depthFrameBuffer = nullptr;
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

void DirectionalLight::initShadowMap(const Vec3& cameraPosition, float nearClip, float farClip, const Size& size)
{
	_nearClip = nearClip;
	_farClip = farClip;

	_shadowMapData.viewMatrix = Mat4::createLookAtWithDirection(
		cameraPosition,
		getDirection(),
		Vec3(0.0f, 1.0f, 0.0f) // とりあえずy方向を上にして固定
	);

	_shadowMapData.projectionMatrix = Mat4::createOrthographicAtCenter(size.width, size.height, nearClip, farClip);

	// デプステクスチャ作成
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.hasShadowMap = 1.0f;

	_shadowMapData.depthTexture = new D3DTexture();
	_shadowMapData.depthTexture->initDepthStencilTexture(size);
#elif defined(MGRRENDERER_USE_OPENGL)
	_hasShadowMap = true;

	_shadowMapData.depthFrameBuffer = new GLFrameBuffer();
	std::vector<GLenum> drawBuffer;
	drawBuffer.push_back(GL_NONE);
	std::vector<GLenum> pixelFormats;
	pixelFormats.push_back(GL_DEPTH_COMPONENT);
	_shadowMapData.depthFrameBuffer->initWithTextureParams(drawBuffer, pixelFormats, false, size);
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
	Logger::logAssert(hasShadowMap(), "prepareShadowMapRendering呼び出しはシャドウマップを使う前提");

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

		direct3dContext->RSSetState(Director::getRenderer().getRasterizeStateCullFaceNormal());
		
		ID3D11RenderTargetView* renderTarget[1] = {nullptr}; // シャドウマップ描画はDepthStencilViewはあるがRenderTargetはないのでnullでいい
		direct3dContext->OMSetRenderTargets(1, renderTarget, _shadowMapData.depthTexture->getDepthStencilView());
		direct3dContext->OMSetDepthStencilState(_shadowMapData.depthTexture->getDepthStencilState(), 1);
#elif defined(MGRRENDERER_USE_OPENGL)
		glBindFramebuffer(GL_FRAMEBUFFER, getShadowMapData().depthFrameBuffer->getFrameBufferId());

		glClear(GL_DEPTH_BUFFER_BIT);

		//TODO:シャドウマップの大きさは画面サイズと同じにしている
		glViewport(0, 0, static_cast<GLsizei>(Director::getInstance()->getWindowSize().width), static_cast<GLsizei>(Director::getInstance()->getWindowSize().height));

		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);
#endif
	});

	Director::getRenderer().addCommand(&_prepareShadowMapRenderingCommand);
}

PointLight::PointLight(const Vec3& position, const Color3B& color, float range) :
#if defined(MGRRENDERER_USE_OPENGL)
_hasShadowMap(false),
#endif
_nearClip(0.0f),
_range(range)
{
	setPosition(position);
	setColor(color);

#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color3F(color) * getIntensity();
	_constantBufferData.position = position;
	_constantBufferData.rangeInverse = 1.0f / range;
#endif
}

void PointLight::setColor(const Color3B& color)
{
	Light::setColor(color);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color3F(color) * getIntensity();
#endif
}

void PointLight::setIntensity(float intensity)
{
	Light::setIntensity(intensity);
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.color = Color3F(getColor()) * intensity;
#endif
}

void PointLight::initShadowMap(float nearClip, float size)
{
	_nearClip = nearClip;

	// x正方向
	_shadowMapData.viewMatrices[0] = Mat4::createLookAtWithDirection(
		getPosition(),
		Vec3(1.0f, 0.0f, 0.0f),
		Vec3(0.0f, 1.0f, 0.0f)
	);

	// x負方向
	_shadowMapData.viewMatrices[1] = Mat4::createLookAtWithDirection(
		getPosition(),
		Vec3(-1.0f, 0.0f, 0.0f),
		Vec3(0.0f, 1.0f, 0.0f)
	);

	// y正方向
	_shadowMapData.viewMatrices[2] = Mat4::createLookAtWithDirection(
		getPosition(),
		Vec3(0.0f, 1.0f, 0.0f),
		Vec3(0.0f, 0.0f, 1.0f)
	);

	// y負方向
	_shadowMapData.viewMatrices[3] = Mat4::createLookAtWithDirection(
		getPosition(),
		Vec3(0.0f, -1.0f, 0.0f),
		Vec3(0.0f, 0.0f, -1.0f)
	);

	// z正方向
	_shadowMapData.viewMatrices[4] = Mat4::createLookAtWithDirection(
		getPosition(),
		Vec3(0.0f, 0.0f, 1.0f),
		Vec3(0.0f, 1.0f, 0.0f)
	);

	// z負方向
	_shadowMapData.viewMatrices[5] = Mat4::createLookAtWithDirection(
		getPosition(),
		Vec3(0.0f, 0.0f, -1.0f),
		Vec3(0.0f, 1.0f, 0.0f)
	);

	// プロジェクション行列は、キューブの中心からある面への四角錘を視錘台としたときの変換
	_shadowMapData.projectionMatrix = Mat4::createPerspective(
		90.0f, // field of view キューブの中心からなので固定で90度
		1.0f, // aspectratio
		nearClip, // near clip
		_range // far clip
	);

	// デプステクスチャ作成
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.hasShadowMap = 1.0f;

	Mat4 lightProjectionMatrix = _shadowMapData.projectionMatrix;
	lightProjectionMatrix = Mat4::CHIRARITY_CONVERTER * lightProjectionMatrix; // 左手系変換行列はプロジェクション行列に最初からかけておく
	lightProjectionMatrix.transpose();
	_constantBufferData.projectionMatrix = lightProjectionMatrix;

	Mat4 depthBiasMatrix = (Mat4::TEXTURE_COORDINATE_CONVERTER * Mat4::createScale(Vec3(0.5f, 0.5f, 1.0f)) * Mat4::createTranslation(Vec3(1.0f, -1.0f, 0.0f))).transpose(); //TODO: Mat4を参照型にすると値がおかしくなってしまう
	_constantBufferData.depthBiasMatrix = depthBiasMatrix;

	for (int i = 0; i < NUM_FACE_CUBEMAP_TEXTURE; i++)
	{
		Mat4 lightViewMatrix = _shadowMapData.viewMatrices[i];
		lightViewMatrix.transpose();
		_constantBufferData.viewMatrices[i] = lightViewMatrix;
	}

	_shadowMapData.depthTexture = new D3DTexture();
	_shadowMapData.depthTexture->initDepthStencilCubeMapTexture(size);
#elif defined(MGRRENDERER_USE_OPENGL)
	_hasShadowMap = true;

	_shadowMapData.depthFrameBuffer = new GLFrameBuffer();
	std::vector<GLenum> drawBuffer;
	drawBuffer.push_back(GL_NONE);
	std::vector<GLenum> pixelFormats;
	pixelFormats.push_back(GL_DEPTH_COMPONENT);
	_shadowMapData.depthFrameBuffer->initWithTextureParams(drawBuffer, pixelFormats, false, Size(size, size));
#endif
}

bool PointLight::hasShadowMap() const {
#if defined(MGRRENDERER_USE_DIRECT3D)
	return _constantBufferData.hasShadowMap > 0.0f;
#elif defined(MGRRENDERER_USE_OPENGL)
	return _hasShadowMap;
#endif
}

void PointLight::prepareShadowMapRendering() {
	Logger::logAssert(hasShadowMap(), "prepareShadowMapRendering呼び出しはシャドウマップを使う前提");

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
		viewport[0].Height = Director::getInstance()->getWindowSize().width; // ポイントライトのシャドウマップは正方形
		viewport[0].MinDepth = 0.0f;
		viewport[0].MaxDepth = 1.0f;
		direct3dContext->RSSetViewports(1, viewport);

		direct3dContext->RSSetState(Director::getRenderer().getRasterizeStateCullFaceNormal());
		
		ID3D11RenderTargetView* renderTarget[1] = {nullptr}; // シャドウマップ描画はDepthStencilViewはあるがRenderTargetはないのでnullでいい
		direct3dContext->OMSetRenderTargets(1, renderTarget, _shadowMapData.depthTexture->getDepthStencilView());
		direct3dContext->OMSetDepthStencilState(_shadowMapData.depthTexture->getDepthStencilState(), 1);
#elif defined(MGRRENDERER_USE_OPENGL)
		glBindFramebuffer(GL_FRAMEBUFFER, getShadowMapData().depthFrameBuffer->getFrameBufferId());

		glClear(GL_DEPTH_BUFFER_BIT);

		//TODO:シャドウマップの大きさは画面サイズと同じにしているが、ポイントライトは正方形
		glViewport(0, 0, static_cast<GLsizei>(Director::getInstance()->getWindowSize().width), static_cast<GLsizei>(Director::getInstance()->getWindowSize().width));

		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);
#endif
	});

	Director::getRenderer().addCommand(&_prepareShadowMapRenderingCommand);
}

SpotLight::SpotLight(const Vec3& position, const Vec3& direction, const Color3B& color, float range, float innerAngle, float outerAngle) :
#if defined(MGRRENDERER_USE_OPENGL)
_hasShadowMap(false),
#endif
_direction(direction),
_nearClip(0.0f),
_range(range),
_innerAngle(innerAngle),
_outerAngle(outerAngle),
_innerAngleCos(cosf(innerAngle)),
_outerAngleCos(cosf(outerAngle))
{
	setPosition(position);
	setColor(color);

#if defined(MGRRENDERER_USE_DIRECT3D)
	Vec3 directionVec = direction;
	directionVec.normalize();
	_constantBufferData.position = position;
	_constantBufferData.rangeInverse = 1.0f / range;
	_constantBufferData.color = Color3F(color) * getIntensity();
	_constantBufferData.innerAngleCos = cosf(innerAngle);
	_constantBufferData.direction = directionVec;
	_constantBufferData.outerAngleCos = cosf(outerAngle);
	_constantBufferData.hasShadowMap = 0.0f;
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

void SpotLight::initShadowMap(float nearClip, const Size& size)
{
	_nearClip = nearClip;

	_shadowMapData.viewMatrix = Mat4::createLookAtWithDirection(
		getPosition(),
		getDirection(),
		Vec3(0.0f, 1.0f, 0.0f) // とりあえずy方向を上にして固定
	);

	_shadowMapData.projectionMatrix = Mat4::createPerspective(
		_outerAngle, // field of view
		size.width / size.height, // aspectratio
		nearClip, // near clip
		_range // far clip
	);

	// デプステクスチャ作成
#if defined(MGRRENDERER_USE_DIRECT3D)
	_constantBufferData.hasShadowMap = 1.0f;

	_shadowMapData.depthTexture = new D3DTexture();
	_shadowMapData.depthTexture->initDepthStencilTexture(size);
#elif defined(MGRRENDERER_USE_OPENGL)
	_hasShadowMap = true;

	_shadowMapData.depthFrameBuffer = new GLFrameBuffer();
	std::vector<GLenum> drawBuffer;
	drawBuffer.push_back(GL_NONE);
	std::vector<GLenum> pixelFormats;
	pixelFormats.push_back(GL_DEPTH_COMPONENT);
	_shadowMapData.depthFrameBuffer->initWithTextureParams(drawBuffer, pixelFormats, false, size);
#endif
}

bool SpotLight::hasShadowMap() const
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	return _constantBufferData.hasShadowMap > 0.0f;
#elif defined(MGRRENDERER_USE_OPENGL)
	return _hasShadowMap;
#endif
}

void SpotLight::prepareShadowMapRendering() {
	Logger::logAssert(hasShadowMap(), "prepareShadowMapRendering呼び出しはシャドウマップを使う前提");

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

		direct3dContext->RSSetState(Director::getRenderer().getRasterizeStateCullFaceNormal());
		
		ID3D11RenderTargetView* renderTarget[1] = {nullptr}; // シャドウマップ描画はDepthStencilViewはあるがRenderTargetはないのでnullでいい
		direct3dContext->OMSetRenderTargets(1, renderTarget, _shadowMapData.depthTexture->getDepthStencilView());
		direct3dContext->OMSetDepthStencilState(_shadowMapData.depthTexture->getDepthStencilState(), 1);
#elif defined(MGRRENDERER_USE_OPENGL)
		glBindFramebuffer(GL_FRAMEBUFFER, getShadowMapData().depthFrameBuffer->getFrameBufferId());

		glClear(GL_DEPTH_BUFFER_BIT);

		//TODO:シャドウマップの大きさは画面サイズと同じにしている
		glViewport(0, 0, static_cast<GLsizei>(Director::getInstance()->getWindowSize().width), static_cast<GLsizei>(Director::getInstance()->getWindowSize().height));

		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);
#endif
	});

	Director::getRenderer().addCommand(&_prepareShadowMapRenderingCommand);
}

} // namespace mgrrenderer
