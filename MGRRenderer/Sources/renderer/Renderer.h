#pragma once
#include "BasicDataTypes.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DProgram.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "GLProgram.h"
#include "GLFrameBuffer.h"
#endif
#include "CustomRenderCommand.h"
#include <vector>
#include <stack>

namespace mgrrenderer
{

class D3DTexture;
class Light;

class Renderer final
{
public:
	enum class RenderMode : int {
		NONE = -1, // 初期値や不正値に使う値
		LIGHTING, // 通常のライティング
		DIFFUSE,
		NORMAL,
		SPECULAR,
	};

	Renderer();
	~Renderer();
#if defined(MGRRENDERER_USE_DIRECT3D)
	void initView(HWND handleWindow, const SizeUint& windowSize);
#elif defined(MGRRENDERER_USE_OPENGL)
	void initView(const SizeUint& windowSize);
#endif

	void toggleDrawWireFrame() { _drawWireFrame = !_drawWireFrame; }
	RenderMode getRenderMode() const { return _renderMode; }
	void setRenderMode(RenderMode mode) { _renderMode = mode; }

#if defined(MGRRENDERER_USE_DIRECT3D)
	IDXGISwapChain* getDirect3dSwapChain() const { return _direct3dSwapChain; }
	ID3D11Device* getDirect3dDevice() const { return _direct3dDevice; }
	ID3D11DeviceContext* getDirect3dContext() const { return _direct3dContext; }
	ID3D11DepthStencilState* getDirect3dDepthStencilState() const { return _direct3dDepthStencilState; };
	ID3D11SamplerState* getPointSamplerState() const { return _pointSampler; }
	ID3D11SamplerState* getLinearSamplerState() const { return _linearSampler; }
	ID3D11SamplerState* getPCFSamplerState() const { return _pcfSampler; }
	ID3D11RasterizerState* getRasterizeStateCullFaceNormal() const { return _rasterizeStateNormal; }
#endif

#if defined(MGRRENDERER_DEFERRED_RENDERING)
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DTexture* getGBufferDepthStencil() const { return _gBufferDepthStencil; }
	D3DTexture* getGBufferColorSpecularIntensity() const { return _gBufferColorSpecularIntensity; }
	D3DTexture* getGBufferNormal() const { return _gBufferNormal; }
	D3DTexture* getGBufferSpecularPower() const { return _gBufferSpecularPower; }
#elif defined(MGRRENDERER_USE_OPENGL)
	GLTexture* getGBufferDepthStencil() const { return _gBufferFrameBuffer->getTextures()[0]; }
	GLTexture* getGBufferColorSpecularIntensity() const { return _gBufferFrameBuffer->getTextures()[1]; }
	GLTexture* getGBufferNormal() const { return _gBufferFrameBuffer->getTextures()[2]; }
	GLTexture* getGBufferSpecularPower() const { return _gBufferFrameBuffer->getTextures()[3]; }
#endif
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

	// TODO:moveコンストラクタ使う？
	void addCommand(RenderCommand* command);
	void render();
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void prepareGBufferRendering();
	void prepareDeferredRendering();
	void renderDeferred();
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)
	void prepareFowardRendering();
	void prepareTransparentRendering();
	void prepareFowardRendering2D();

private:
	// stdにはtreeがないのでtreeのようなトラバース方法をstackとvectorで実現している
	// TODO:専用の効率いいtreeの実装。
	std::stack<size_t> _groupIndexStack;
	// TODO:moveコンストラクタ使う？
	std::vector<std::vector<RenderCommand*>> _queueGroup;
	Quadrangle2D _quadrangle;
	// ワイアーフレームのみ描画するモード
	bool _drawWireFrame;
	RenderMode _renderMode;
#if defined(MGRRENDERER_USE_DIRECT3D)
	IDXGISwapChain* _direct3dSwapChain;
	ID3D11Device* _direct3dDevice;
	ID3D11DeviceContext* _direct3dContext;
	ID3D11RenderTargetView* _direct3dRenderTarget;
	ID3D11DepthStencilView* _direct3dDepthStencilView;
	ID3D11DepthStencilState* _direct3dDepthStencilState;
	ID3D11DepthStencilState* _direct3dDepthStencilStateTransparent;
	ID3D11DepthStencilState* _direct3dDepthStencilState2D;
	D3D11_VIEWPORT _direct3dViewport[1];
	ID3D11SamplerState* _pointSampler;
	ID3D11SamplerState* _linearSampler;
	ID3D11SamplerState* _pcfSampler;
	ID3D11RasterizerState* _rasterizeStateNormal;
	ID3D11RasterizerState* _rasterizeStateWireFrame;
	ID3D11BlendState* _blendState;
	ID3D11BlendState* _blendStateTransparent;
#endif

#if defined(MGRRENDERER_DEFERRED_RENDERING)
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DTexture* _gBufferDepthStencil;
	D3DTexture* _gBufferColorSpecularIntensity;
	D3DTexture* _gBufferNormal;
	D3DTexture* _gBufferSpecularPower;
	D3DProgram _d3dProgramForDeferredRendering;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLFrameBuffer* _gBufferFrameBuffer;
	GLProgram _glProgramForDeferredRendering;
#endif
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

	void prepareDefaultRenderTarget();
	void visitRenderQueue(const std::vector<RenderCommand*> queue);
	void executeRenderCommand(RenderCommand* command);
};

} // namespace mgrrenderer
