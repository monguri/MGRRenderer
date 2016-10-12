#include "Director.h"
#include "Image.h"
#include "TextureUtility.h"
#include "FPSFontImage.h"
#include "LabelAtlas.h"
#include "Sprite2D.h"
#if defined(MGRRENDERER_USE_OPENGL)
#include "GLTexture.h"
#endif

namespace mgrrenderer
{

Director* Director::_instance = nullptr;

Director::Director() :
#if defined(MGRRENDERER_USE_DIRECT3D)
_direct3dDevice(nullptr),
_direct3dContext(nullptr),
_direct3dRenderTarget(nullptr),
_direct3dDepthStencilView(nullptr),
_direct3dDepthStencilState(nullptr),
_direct3dDepthStencilState2D(nullptr),
_displayGBuffer(false),
_gBufferDepthStencil(nullptr),
_gBufferColorSpecularIntensitySprite(nullptr),
_gBufferNormal(nullptr),
_gBufferSpecularPower(nullptr),
#endif
_displayStats(false),
_accumulatedDeltaTime(0.0f),
_FPSLabel(nullptr),
_nearClip(0.0f),
_farClip(0.0f)
{
}

Director::~Director()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	if (_gBufferSpecularPower != nullptr)
	{
		delete _gBufferSpecularPower;
		_gBufferSpecularPower = nullptr;
	}

	if (_gBufferNormal != nullptr)
	{
		delete _gBufferNormal;
		_gBufferNormal = nullptr;
	}

	if (_gBufferColorSpecularIntensitySprite != nullptr)
	{
		delete _gBufferColorSpecularIntensitySprite;
		_gBufferColorSpecularIntensitySprite = nullptr;
	}

	if (_gBufferDepthStencil != nullptr)
	{
		delete _gBufferDepthStencil;
		_gBufferDepthStencil = nullptr;
	}
#endif

	if (_FPSLabel != nullptr)
	{
		delete _FPSLabel;
		_FPSLabel = nullptr;
	}

	_instance = nullptr;
}

Director* Director::getInstance()
{
	if (_instance == nullptr)
	{
		_instance = new Director();
	}

	return _instance;
}

void Director::destroy()
{
	delete _instance;
}

void Director::init(const Size& windowSize, float nearClip, float farClip)
{
	calculateDeltaTime();

	_windowSize = windowSize;
	_nearClip = nearClip;
	_farClip = farClip;

	// TODO:後で描画関係の初期化処理はこの中にまとめる
	_renderer.initView(windowSize);

#if defined(MGRRENDERER_USE_OPENGL)
	Logger::log("GPU vendor: %s\nGPU:%s\nOpenGL version:%s\nGLSLversion:%s", glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

	// デフォルトのピクセルフォーマットをRGBA8888に。
	GLTexture::setDefaultPixelFormat(TextureUtility::PixelFormat::RGBA8888);
#endif

	createStatsLabel();
	createGBufferSprite();
}

void Director::setScene(const Scene& scene)
{
	// Sceneはサイズの大きなstd::vectorを含むのでコピーコンストラクトさせたくないのでmoveする。
	_scene = std::move(scene);
}

void Director::update()
{
	float dt = calculateDeltaTime();

	_scene.update(dt);

	// 現状、デバッグ表示は2Dノードなので、Scene:updateの最後に必要な設定は済んでいる
	if (_displayStats)
	{
		updateStats(dt);
	}

	if (_displayGBuffer)
	{
		renderGBufferSprite();
	}

	_renderer.render();
}

Renderer& Director::getRenderer()
{
	return getInstance()->_renderer;
}

Camera& Director::getCamera()
{
	return getInstance()->getScene().getCamera();
}

const Camera& Director::getCameraFor2D()
{
	return getInstance()->getScene().getCameraFor2D();
}

const std::vector<Light*>& Director::getLight()
{
	return getInstance()->getScene().getLight();
}

struct timeval Director::getCurrentTimeOfDay()
{
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);

	struct timeval ret;
	ret.tv_sec = static_cast<long>(count.QuadPart / freq.QuadPart);
	ret.tv_usec = static_cast<long>(count.QuadPart * 1000000 / freq.QuadPart - ret.tv_sec * 1000000);
	return ret;
}

float Director::calculateDeltaTime()
{
	const struct timeval& now = getCurrentTimeOfDay();
	float ret = now.tv_sec - _lastUpdateTime.tv_sec + (now.tv_usec - _lastUpdateTime.tv_usec) / 1000000.0f;
	_lastUpdateTime = now;
	return max(0.0f, ret);
}

void Director::createStatsLabel()
{
	if (_FPSLabel == nullptr)
	{
		_FPSLabel = new (std::nothrow) LabelAtlas();
	}

	//Texture::PixelFormat currentFormat = 
	Image image; // ImageはCPU側のメモリを使っているのでこのスコープで解放されてもよいものだからスタックに取る
	bool success = image.initWithImageData(FPSFontImage::PNG_DATA, FPSFontImage::getPngDataSize());
	Logger::logAssert(success, "Imageの初期化に失敗");

#if defined(MGRRENDERER_USE_OPENGL)
	GLTexture* texture = new (std::nothrow) GLTexture(); // TextureはGPU側のメモリを使ってるので解放されると困るのでヒープにとる
	success = texture->initWithImage(image, TextureUtility::PixelFormat::RGBA4444);
	Logger::logAssert(success, "Textureの初期化に失敗");

	_FPSLabel->init("", texture,
		12, 32, '.'); // 左の情報は、すでにテクスチャの情報を知っていることからの決め打ち
	_FPSLabel->setPosition(Vec3(0, 0, 0));
#endif
}

void Director::createGBufferSprite()
{
	_gBufferDepthStencil = new Sprite2D();
#if defined(MGRRENDERER_USE_DIRECT3D)
	_gBufferDepthStencil->initWithRenderBuffer(_renderer.getGBufferDepthStencil(), Sprite2D::RenderBufferType::DEPTH_TEXTURE);
#elif defined(MGRRENDERER_USE_OPENGL)
	_gBufferDepthStencil->initWithRenderBuffer(_renderer.getGBuffers()[0], Sprite2D::RenderBufferType::DEPTH_TEXTURE);
#endif
	_gBufferDepthStencil->setScale(1 / 5.0f);
	_gBufferDepthStencil->setPosition(Vec3(0.0f, 0.0f, 0.0f));

	_gBufferColorSpecularIntensitySprite = new Sprite2D();
#if defined(MGRRENDERER_USE_DIRECT3D)
	_gBufferColorSpecularIntensitySprite->initWithRenderBuffer(_renderer.getGBufferColorSpecularIntensity(), Sprite2D::RenderBufferType::GBUFFER_COLOR_SPECULAR_INTENSITY);
#elif defined(MGRRENDERER_USE_OPENGL)
	_gBufferColorSpecularIntensitySprite->initWithRenderBuffer(_renderer.getGBuffers()[1], Sprite2D::RenderBufferType::GBUFFER_COLOR_SPECULAR_INTENSITY);
#endif
	_gBufferColorSpecularIntensitySprite->setScale(1 / 5.0f);
	_gBufferColorSpecularIntensitySprite->setPosition(Vec3(_windowSize.width / 5.0f, 0.0f, 0.0f));

	_gBufferNormal = new Sprite2D();
#if defined(MGRRENDERER_USE_DIRECT3D)
	_gBufferNormal->initWithRenderBuffer(_renderer.getGBufferNormal(), Sprite2D::RenderBufferType::GBUFFER_NORMAL);
#elif defined(MGRRENDERER_USE_OPENGL)
	_gBufferNormal->initWithRenderBuffer(_renderer.getGBuffers()[2], Sprite2D::RenderBufferType::GBUFFER_NORMAL);
#endif
	_gBufferNormal->setScale(1 / 5.0f);
	_gBufferNormal->setPosition(Vec3(_windowSize.width / 5.0f * 2, 0.0f, 0.0f));

	_gBufferSpecularPower = new Sprite2D();
#if defined(MGRRENDERER_USE_DIRECT3D)
	_gBufferSpecularPower->initWithRenderBuffer(_renderer.getGBufferSpecularPower(), Sprite2D::RenderBufferType::GBUFFER_SPECULAR_POWER);
#elif defined(MGRRENDERER_USE_OPENGL)
	_gBufferSpecularPower->initWithRenderBuffer(_renderer.getGBuffers()[3], Sprite2D::RenderBufferType::GBUFFER_SPECULAR_POWER);
#endif
	_gBufferSpecularPower->setScale(1 / 5.0f);
	_gBufferSpecularPower->setPosition(Vec3(_windowSize.width / 5.0f * 3, 0.0f, 0.0f));
}

void Director::renderGBufferSprite()
{
	// addChildしてないので直接描画する
	_gBufferDepthStencil->prepareRendering();
	_gBufferColorSpecularIntensitySprite->prepareRendering();
	_gBufferNormal->prepareRendering();
	_gBufferSpecularPower->prepareRendering();

	_gBufferDepthStencil->renderForward();
	_gBufferColorSpecularIntensitySprite->renderForward();
	_gBufferNormal->renderForward();
	_gBufferSpecularPower->renderForward();
}

void Director::updateStats(float dt)
{
	static float prevDeltaTime = 0.016f; // 初期値は60FPS
	static const float FPS_FILTER = 0.10f;
	static const float STATS_INTERVAL = 0.10f; // 0.1秒ごとにfps更新

	_accumulatedDeltaTime += dt;

	// cocosの真似だが、現在のdtが10%で、60FPSを90%で重ねるので現在のdtの影響が結構小さい.
	float avgDeltaTime = dt * FPS_FILTER + prevDeltaTime * (1.0f - FPS_FILTER);
	prevDeltaTime = avgDeltaTime;
	float fps = 1.0f / avgDeltaTime;

	if (_accumulatedDeltaTime > STATS_INTERVAL)
	{
		if (_FPSLabel != nullptr)
		{
			char buffer[30]; // 30はcocosのshowStatsの真似

			sprintf_s(buffer, "%.1f / %.3f", fps, avgDeltaTime);
#if defined(MGRRENDERER_USE_OPENGL)
			_FPSLabel->setString(buffer);
#endif
		}

		//Logger::log("%.1f / %.3f", fps, avgDeltaTime);
		_accumulatedDeltaTime = 0.0f;
	}

	if (_FPSLabel != nullptr)
	{
		// FPSラベルはどこにもaddChildしないのでここでvisitを呼んで描画する
		// 影は関係ないので一気に全パス描画してしまう
		_FPSLabel->update(dt);
		_FPSLabel->prepareRendering();
		_FPSLabel->renderShadowMap();
		_FPSLabel->renderForward();
	}
}

} // namespace mgrrenderer
