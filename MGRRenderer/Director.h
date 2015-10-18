#pragma once
#include "Scene.h"

namespace mgrrenderer
{
	
class Director
{
public:
	static Director* getInstance();
	void destroy();
	void initWithScene(const Scene& scene);
	void update();

private:
	static Director* _instance;
	Scene _scene;
};

} // namespace mgrrenderer
