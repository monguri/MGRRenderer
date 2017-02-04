#pragma once
#include "Config.h"
#include "node/Scene.h"
#include "Renderer.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include <d3d11.h>
#endif

namespace mgrrenderer
{
class Camera;
class LabelAtlas;
class Sprite2D;
class Light;

class Director
{
public:
	static Director* getInstance();
	void destroy();
	// 正のnearClip、farClipを渡すと、右手系でニアクリップとファークリップのz座標は負として扱うので注意
#if defined(MGRRENDERER_USE_DIRECT3D)
	void init(HWND handleWindow, const SizeUint& windowSize, float nearClip, float farClip);
#elif defined(MGRRENDERER_USE_OPENGL)
	void init(const SizeUint& windowSize, float nearClip, float farClip);
#endif
	const SizeUint& getWindowSize() const { return _windowSize; }
	const float getNearClip() const { return _nearClip; }
	const float getFarClip() const { return _farClip; }
	Scene& getScene() { return _scene; }
	void setScene(const Scene& scene);
	void update();
	static Renderer& getRenderer();
	static Camera& getCamera();
	static const Camera& getCameraFor2D();

	void setDisplayStats(bool displayStats) { _displayStats = displayStats; }
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void setDisplayGBuffer(bool displayGBuffer) { _displayGBuffer = displayGBuffer; }
#endif

private:
	static Director* _instance;
	Scene _scene;
	Renderer _renderer;
	SizeUint _windowSize;
	float _nearClip;
	float _farClip;
	struct timeval _lastUpdateTime;
	bool _displayStats;
	float _accumulatedDeltaTime;
	LabelAtlas* _FPSLabel;
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	// Gバッファのデバッグ描画
	bool _displayGBuffer;
	Sprite2D* _gBufferDepthStencil;
	Sprite2D* _gBufferColorSpecularIntensitySprite;
	Sprite2D* _gBufferNormal;
	Sprite2D* _gBufferSpecularPower;
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

	Director();
	~Director();
	struct timeval getCurrentTimeOfDay();
	float calculateDeltaTime();
	void updateStats(float dt);
	void createStatsLabel();
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void initGBufferSprite();
	void clearGBufferSprite();
	void renderGBufferSprite();
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)
};

} // namespace mgrrenderer
