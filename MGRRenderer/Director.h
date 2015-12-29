#pragma once
#include "Scene.h"

namespace mgrrenderer
{
	
class Camera;

class Director
{
public:
	static Director* getInstance();
	void destroy();
	void init(const Size& windowSize);
	const Size& getWindowSize() const { return _windowSize; }
	Scene& getScene() { return _scene; }
	void setScene(const Scene& scene);
	void update();
	static Camera& getCamera();

private:
	static Director* _instance;
	Scene _scene;
	Size _windowSize;
	struct timeval _lastUpdateTime;

	struct timeval getCurrentTimeOfDay();
	float calculateDeltaTime();
};

} // namespace mgrrenderer
