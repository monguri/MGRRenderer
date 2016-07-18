#pragma once
#include "BasicDataTypes.h"
#include <vector>
#include <stack>

namespace mgrrenderer
{

class RenderCommand;
class D3DTexture;

class Renderer final
{
public:
	Renderer();
	~Renderer();
	void initView(const Size& windowSize);

	// TODO:moveコンストラクタ使う？
	void addCommand(RenderCommand* command);
	void render();
	void prepareDifferedRendering();
	static void prepareFowardRendering();

private:
	// stdにはtreeがないのでtreeのようなトラバース方法をstackとvectorで実現している
	// TODO:専用の効率いいtreeの実装。
	std::stack<size_t> _groupIndexStack;
	// TODO:moveコンストラクタ使う？
	std::vector<std::vector<RenderCommand*>> _queueGroup;
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DTexture* _gBufferDepthStencil;
	D3DTexture* _gBufferColorSpecularIntensity;
	D3DTexture* _gBufferNormal;
	D3DTexture* _gBufferSpecularPower;
#endif

	void visitRenderQueue(const std::vector<RenderCommand*> queue);
	void executeRenderCommand(RenderCommand* command);
};

} // namespace mgrrenderer
