#include "Director.h"
#include "Image.h"
#include "TextureUtility.h"
#include "FPSFontImage.h"
#include "LabelAtlas.h"
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
_direct3dDepthStencil(nullptr),
#endif
_displayStats(false),
_accumulatedDeltaTime(0.0f),
_FPSLabel(nullptr)
{
}

Director::~Director()
{
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

void Director::init(const Size& windowSize)
{
	calculateDeltaTime();

	_windowSize = windowSize;

	// TODO:��ŕ`��֌W�̏����������͂��̒��ɂ܂Ƃ߂�
	_renderer.initView(windowSize);

#if defined(MGRRENDERER_USE_OPENGL)
	Logger::log("GPU vendor: %s\nGPU:%s\nOpenGL version:%s\nGLSLversion:%s", glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

	// �f�t�H���g�̃s�N�Z���t�H�[�}�b�g��RGBA8888�ɁB
	GLTexture::setDefaultPixelFormat(TextureUtility::PixelFormat::RGBA8888);
#endif

	createStatsLabel();
}

void Director::setScene(const Scene& scene)
{
	// Scene�̓T�C�Y�̑傫��std::vector���܂ނ̂ŃR�s�[�R���X�g���N�g���������Ȃ��̂�move����B
	_scene = std::move(scene);
}

void Director::update()
{
	float dt = calculateDeltaTime();

	_scene.update(dt);

	if (_displayStats)
	{
		updateStats(dt);
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
	ret.tv_sec = count.QuadPart / freq.QuadPart;
	ret.tv_usec = count.QuadPart * 1000000 / freq.QuadPart - ret.tv_sec * 1000000;
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
	Image image; // Image��CPU���̃��������g���Ă���̂ł��̃X�R�[�v�ŉ������Ă��悢���̂�����X�^�b�N�Ɏ��
	bool success = image.initWithImageData(FPSFontImage::PNG_DATA, FPSFontImage::getPngDataSize());
	Logger::logAssert(success, "Image�̏������Ɏ��s");

#if defined(MGRRENDERER_USE_OPENGL)
	GLTexture* texture = new (std::nothrow) GLTexture(); // Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
	success = texture->initWithImage(image, TextureUtility::PixelFormat::RGBA4444);
	Logger::logAssert(success, "Texture�̏������Ɏ��s");

	_FPSLabel->init("", texture,
		12, 32, '.'); // ���̏��́A���łɃe�N�X�`���̏���m���Ă��邱�Ƃ���̌��ߑł�
	_FPSLabel->setPosition(Vec3(0, 0, 0));
#endif
}

void Director::updateStats(float dt)
{
	static float prevDeltaTime = 0.016f; // �����l��60FPS
	static const float FPS_FILTER = 0.10f;
	static const float STATS_INTERVAL = 0.10f; // 0.1�b���Ƃ�fps�X�V

	_accumulatedDeltaTime += dt;

	// cocos�̐^�������A���݂�dt��10%�ŁA60FPS��90%�ŏd�˂�̂Ō��݂�dt�̉e�������\������.
	float avgDeltaTime = dt * FPS_FILTER + prevDeltaTime * (1.0f - FPS_FILTER);
	prevDeltaTime = avgDeltaTime;
	float fps = 1.0f / avgDeltaTime;

	if (_accumulatedDeltaTime > STATS_INTERVAL)
	{
		if (_FPSLabel != nullptr)
		{
			char buffer[30]; // 30��cocos��showStats�̐^��

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
		// FPS���x���͂ǂ��ɂ�addChild���Ȃ��̂ł�����visit���Ă�ŕ`�悷��
		// �e�͊֌W�Ȃ��̂ň�C�ɑS�p�X�`�悵�Ă��܂�
#if defined(MGRRENDERER_USE_OPENGL)
		glDisable(GL_DEPTH_TEST);
#endif
		_FPSLabel->update(dt);
		_FPSLabel->renderShadowMap();
		_FPSLabel->renderWithShadowMap();
#if defined(MGRRENDERER_USE_OPENGL)
		glEnable(GL_DEPTH_TEST);
#endif
	}
}

} // namespace mgrrenderer
