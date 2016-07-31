#pragma once
#include "Config.h"
#include "Scene.h"
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
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11Device* getDirect3dDevice() const { return _direct3dDevice; }
	void setDirect3dDevice(ID3D11Device* device) { _direct3dDevice = device; }
	ID3D11DeviceContext* getDirect3dContext() const { return _direct3dContext; }
	void setDirect3dContext(ID3D11DeviceContext* context) { _direct3dContext = context; }
	ID3D11RenderTargetView* getDirect3dRenderTarget() const { return _direct3dRenderTarget; }
	void setDirect3dRenderTarget(ID3D11RenderTargetView* target) { _direct3dRenderTarget = target; }
	ID3D11DepthStencilView* getDirect3dDepthStencilView() const { return _direct3dDepthStencilView; }
	void setDirect3dDepthStencilView(ID3D11DepthStencilView* view) { _direct3dDepthStencilView = view; }
	ID3D11DepthStencilState* getDirect3dDepthStencilState() const { return _direct3dDepthStencilState; }
	void setDirect3dDepthStencilState(ID3D11DepthStencilState* state) { _direct3dDepthStencilState = state; }
	ID3D11DepthStencilState* getDirect3dDepthStencilState2D() const { return _direct3dDepthStencilState2D; }
	void setDirect3dDepthStencilState2D(ID3D11DepthStencilState* state) { _direct3dDepthStencilState2D = state; }
	D3D11_VIEWPORT* getDirect3dViewport() const { return _direct3dViewport; }
	void setDirect3dViewport(D3D11_VIEWPORT* viewport) { _direct3dViewport = viewport; }
#endif
	void destroy();
	void init(const Size& windowSize);
	const Size& getWindowSize() const { return _windowSize; }
	Scene& getScene() { return _scene; }
	void setScene(const Scene& scene);
	void update();
	static Renderer& getRenderer();
	static Camera& getCamera();
	static const Camera& getCameraFor2D();
	static const std::vector<Light*>& getLight();

	void setDisplayStats(bool displayStats) { _displayStats = displayStats; }
#if defined(MGRRENDERER_USE_DIRECT3D)
	void setDisplayGBuffer(bool displayGBuffer) { _displayGBuffer = displayGBuffer; }
#endif

private:
	static Director* _instance;
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11Device* _direct3dDevice;
	ID3D11DeviceContext* _direct3dContext;
	ID3D11RenderTargetView* _direct3dRenderTarget;
	ID3D11DepthStencilView* _direct3dDepthStencilView;
	ID3D11DepthStencilState* _direct3dDepthStencilState;
	ID3D11DepthStencilState* _direct3dDepthStencilState2D;
	D3D11_VIEWPORT* _direct3dViewport;
#endif
	Scene _scene;
	Renderer _renderer;
	Size _windowSize;
	struct timeval _lastUpdateTime;
	bool _displayStats;
	float _accumulatedDeltaTime;
	LabelAtlas* _FPSLabel;
	// G�o�b�t�@�̃f�o�b�O�`��
#if defined(MGRRENDERER_USE_DIRECT3D)
	bool _displayGBuffer;
	Sprite2D* _gBufferDepthStencil;
	Sprite2D* _gBufferColorSpecularIntensitySprite;
	Sprite2D* _gBufferNormal;
	Sprite2D* _gBufferSpecularPower;
#endif

	Director();
	~Director();
	struct timeval getCurrentTimeOfDay();
	float calculateDeltaTime();
	void updateStats(float dt);
	void createStatsLabel();
#if defined(MGRRENDERER_USE_DIRECT3D)
	void createGBufferSprite();
	void renderGBufferSprite();
#endif
};

} // namespace mgrrenderer
