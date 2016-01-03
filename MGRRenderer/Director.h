#pragma once
#include "Scene.h"

namespace mgrrenderer
{
	
class Camera;
class LabelAtlas;

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
	void setDisplayStats(bool displayStats) { _displayStats = displayStats; }

private:
	static Director* _instance;
	Scene _scene;
	Size _windowSize;
	struct timeval _lastUpdateTime;
	bool _displayStats;
	float _accumulatedDeltaTime;
	LabelAtlas* _FPSLabel;

	Director();
	~Director();
	struct timeval getCurrentTimeOfDay();
	float calculateDeltaTime();
	void updateStats(float dt);
	void createStatsLabel();
};

} // namespace mgrrenderer
