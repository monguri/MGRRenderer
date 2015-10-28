#pragma once
#include "Scene.h"

namespace mgrrenderer
{
	
class Director
{
public:
	static Director* getInstance();
	void destroy();
	void init(const Size& windowSize);
	void setScene(const Scene& scene);
	void update();
	const Size& getWindowSize() { return _windowSize; }

private:
	static Director* _instance;
	Scene _scene;
	Size _windowSize;
};

} // namespace mgrrenderer
