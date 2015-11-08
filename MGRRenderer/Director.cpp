#include "Director.h"

namespace mgrrenderer
{

Director* Director::_instance = nullptr;

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
	_scene.update();
}

Camera& Director::getCamera()
{
	return getInstance()->getScene().getCamera();
}

} // namespace mgrrenderer
