#include "Director.h"
#include "Image.h"
#include "TextureUtility.h"
#include "embeddata/FPSFontImage.h"
#include "node/LabelAtlas.h"
#include "node/Sprite2D.h"
#if defined(MGRRENDERER_USE_OPENGL)
#include "GLTexture.h"
#endif

namespace mgrrenderer
{

Director* Director::_instance = nullptr;

Director::Director() :
#if defined(MGRRENDERER_DEFERRED_RENDERING)
_displayGBuffer(false),
_gBufferDepthStencil(nullptr),
_gBufferColorSpecularIntensitySprite(nullptr),
_gBufferNormal(nullptr),
_gBufferSpecularPower(nullptr),
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)
_displayStats(false),
_accumulatedDeltaTime(0.0f),
_FPSLabel(nullptr),
_nearClip(0.0f),
_farClip(0.0f)
{
}

Director::~Director()
{
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	clearGBufferSprite();
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

#if defined(MGRRENDERER_USE_DIRECT3D)
void Director::init(HWND handleWindow, const SizeUint& windowSize, float nearClip, float farClip)
#elif defined(MGRRENDERER_USE_OPENGL)
void Director::init(const SizeUint& windowSize, float nearClip, float farClip)
#endif
{
	calculateDeltaTime();

	_windowSize = windowSize;
	_nearClip = nearClip;
	_farClip = farClip;

#if defined(MGRRENDERER_USE_DIRECT3D)
	_renderer.initView(handleWindow, windowSize);
#elif defined(MGRRENDERER_USE_OPENGL)
	_renderer.initView(windowSize);
#endif

#if defined(MGRRENDERER_USE_OPENGL)
	Logger::log("GPU vendor: %s\nGPU:%s\nOpenGL version:%s\nGLSLversion:%s", glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

	GLint numExtensions = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	Logger::log("================OpenGL Extension, from this line.=================");
	for (int i = 0; i < numExtensions; i++)
	{
		Logger::log("%s", glGetStringi(GL_EXTENSIONS, i));
	}
	Logger::log("================OpenGL Extension to this line.====================");

	// デフォルトのピクセルフォーマットをRGBA8888に。
	GLTexture::setDefaultPixelFormat(TextureUtility::PixelFormat::RGBA8888);
#endif

	createStatsLabel();
}

void Director::setScene(const Scene& scene)
{
	// Sceneはサイズの大きなstd::vectorを含むのでコピーコンストラクトさせたくないのでmoveする。
	_scene = std::move(scene);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	// Gバッファのデプスのプロジェクション行列はシーンに設定したカメラの設定に依存するのでここでGバッファを初期化する
	initGBufferSprite();
#endif
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

#if defined(MGRRENDERER_DEFERRED_RENDERING)
	if (_displayGBuffer)
	{
		renderGBufferSprite();
	}
#endif

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

#if defined(MGRRENDERER_DEFERRED_RENDERING)
void Director::initGBufferSprite()
{
	clearGBufferSprite();

	if (_renderer.getGBufferDepthStencil() != nullptr) // デプスステンシルにはレンダーバッファを使ってテクスチャを使わなかった場合は表示しない
	{
		_gBufferDepthStencil = new Sprite2D();
		_gBufferDepthStencil->initWithDepthStencilTexture(_renderer.getGBufferDepthStencil(), Sprite2D::RenderBufferType::DEPTH_TEXTURE, getNearClip(), getFarClip(), getCamera().getProjectionMatrix());
	}

	if (_gBufferDepthStencil != nullptr)
	{
		_gBufferDepthStencil->setScale(1 / 5.0f);
		_gBufferDepthStencil->setPosition(Vec3(0.0f, 0.0f, 0.0f));
	}

	_gBufferColorSpecularIntensitySprite = new Sprite2D();
	_gBufferColorSpecularIntensitySprite->initWithRenderBuffer(_renderer.getGBufferColorSpecularIntensity(), Sprite2D::RenderBufferType::GBUFFER_COLOR_SPECULAR_INTENSITY);
	_gBufferColorSpecularIntensitySprite->setScale(1 / 5.0f);
	_gBufferColorSpecularIntensitySprite->setPosition(Vec3(_windowSize.width / 5.0f, 0.0f, 0.0f));

	_gBufferNormal = new Sprite2D();
	_gBufferNormal->initWithRenderBuffer(_renderer.getGBufferNormal(), Sprite2D::RenderBufferType::GBUFFER_NORMAL);
	_gBufferNormal->setScale(1 / 5.0f);
	_gBufferNormal->setPosition(Vec3(_windowSize.width / 5.0f * 2, 0.0f, 0.0f));

	_gBufferSpecularPower = new Sprite2D();
	_gBufferSpecularPower->initWithRenderBuffer(_renderer.getGBufferSpecularPower(), Sprite2D::RenderBufferType::GBUFFER_SPECULAR_POWER);
	_gBufferSpecularPower->setScale(1 / 5.0f);
	_gBufferSpecularPower->setPosition(Vec3(_windowSize.width / 5.0f * 3, 0.0f, 0.0f));
}

void Director::clearGBufferSprite()
{
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
}

void Director::renderGBufferSprite()
{
	// addChildしてないので直接描画する
	if (_gBufferDepthStencil != nullptr)
	{
		_gBufferDepthStencil->prepareRendering();
	}
	_gBufferColorSpecularIntensitySprite->prepareRendering();
	_gBufferNormal->prepareRendering();
	_gBufferSpecularPower->prepareRendering();

	if (_gBufferDepthStencil != nullptr)
	{
		_gBufferDepthStencil->renderForward();
	}
	_gBufferColorSpecularIntensitySprite->renderForward();
	_gBufferNormal->renderForward();
	_gBufferSpecularPower->renderForward();
}
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

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
		_FPSLabel->renderForward();
	}
}

} // namespace mgrrenderer
