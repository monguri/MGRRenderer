#include "Director.h"

namespace mgrrenderer
{

Director* Director::_instance = nullptr;

Director::Director() : _displayStats(false), _accumulatedDeltaTime(0.0f)
{

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
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	// TODO:2D描画しかしないときもデプステストをONにしている
	glEnable(GL_DEPTH_TEST);
	// OpenGL側でやるビューポート変換のためのパラメータを渡す
	glViewport(0, 0, windowSize.width, windowSize.height);
}

void Director::setScene(const Scene& scene)
{
	// Sceneはサイズの大きなstd::vectorを含むのでコピーコンストラクトさせたくないのでmoveする。
	_scene = std::move(scene);
}

void Director::update()
{
	float dt = calculateDeltaTime();

	if (_displayStats)
	{
		updateStats(dt);
	}

	_scene.update(dt);
}

Camera& Director::getCamera()
{
	return getInstance()->getScene().getCamera();
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
		//char buffer[30]; // 30はcocosのshowStatsの真似

		//sprintf_s(buffer, "%.1f", fps);
		Logger::log("%.1f / %.3f", fps, avgDeltaTime);
		_accumulatedDeltaTime = 0.0f;
	}
}

} // namespace mgrrenderer
