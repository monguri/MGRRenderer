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
	Renderer();
	~Renderer();
	void initView(const Size& windowSize);

#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11SamplerState* getPointSamplerState() const { return _pointSampler; }
	ID3D11SamplerState* getLinearSamplerState() const { return _linearSampler; }
	ID3D11SamplerState* getPCFSamplerState() const { return _pcfSampler; }
	ID3D11RasterizerState* getRasterizeStateCullFaceNormal() const { return _rasterizeStateNormal; }
	ID3D11RasterizerState* getRasterizeStateCullFaceFront() const { return _rasterizeStateCullFaceFront; }
	ID3D11RasterizerState* getRasterizeStateCullFaceBack() const { return _rasterizeStateCullFaceBack; }
#endif

#if defined(MGRRENDERER_DEFERRED_RENDERING)
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DTexture* getGBufferDepthStencil() const { return _gBufferDepthStencil; }
	D3DTexture* getGBufferColorSpecularIntensity() const { return _gBufferColorSpecularIntensity; }
	D3DTexture* getGBufferNormal() const { return _gBufferNormal; }
	D3DTexture* getGBufferSpecularPower() const { return _gBufferSpecularPower; }
#elif defined(MGRRENDERER_USE_OPENGL)
	const std::vector<GLTexture*>& getGBuffers() const { return _gBufferFrameBuffer->getTextures(); }
#endif
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

	// TODO:moveコンストラクタ使う？
	void addCommand(RenderCommand* command);
	void render();
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void prepareGBufferRendering();
	static void prepareDeferredRendering();
	void renderDeferred();
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)
	static void prepareFowardRendering();
	static void prepareFowardRendering2D();

private:
	// stdにはtreeがないのでtreeのようなトラバース方法をstackとvectorで実現している
	// TODO:専用の効率いいtreeの実装。
	std::stack<size_t> _groupIndexStack;
	// TODO:moveコンストラクタ使う？
	std::vector<std::vector<RenderCommand*>> _queueGroup;
	Quadrangle2D _quadrangle;
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11SamplerState* _pointSampler;
	ID3D11SamplerState* _linearSampler;
	ID3D11SamplerState* _pcfSampler;
	ID3D11RasterizerState* _rasterizeStateNormal;
	ID3D11RasterizerState* _rasterizeStateCullFaceFront;
	ID3D11RasterizerState* _rasterizeStateCullFaceBack;
#endif

#if defined(MGRRENDERER_DEFERRED_RENDERING)
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DTexture* _gBufferDepthStencil;
	D3DTexture* _gBufferColorSpecularIntensity;
	D3DTexture* _gBufferNormal;
	D3DTexture* _gBufferSpecularPower;
	D3DProgram _d3dProgram;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLFrameBuffer* _gBufferFrameBuffer;
	GLProgram _glProgram;
#endif
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

	void visitRenderQueue(const std::vector<RenderCommand*> queue);
	void executeRenderCommand(RenderCommand* command);
};

} // namespace mgrrenderer
