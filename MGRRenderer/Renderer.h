#pragma once
#include "BasicDataTypes.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DProgram.h"
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
	D3DTexture* getGBufferDepthStencil() const { return _gBufferDepthStencil; }
	D3DTexture* getGBufferColorSpecularIntensity() const { return _gBufferColorSpecularIntensity; }
	D3DTexture* getGBufferNormal() const { return _gBufferNormal; }
	D3DTexture* getGBufferSpecularPower() const { return _gBufferSpecularPower; }
#endif

	// TODO:move�R���X�g���N�^�g���H
	void addCommand(RenderCommand* command);
	void render();
	void prepareGBufferRendering();
	static void prepareDeferredRendering();
	void renderDeferred();
	static void prepareFowardRendering();
	static void prepareFowardRendering2D();

private:
	// std�ɂ�tree���Ȃ��̂�tree�̂悤�ȃg���o�[�X���@��stack��vector�Ŏ������Ă���
	// TODO:��p�̌�������tree�̎����B
	std::stack<size_t> _groupIndexStack;
	// TODO:move�R���X�g���N�^�g���H
	std::vector<std::vector<RenderCommand*>> _queueGroup;
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DTexture* _gBufferDepthStencil;
	D3DTexture* _gBufferColorSpecularIntensity;
	D3DTexture* _gBufferNormal;
	D3DTexture* _gBufferSpecularPower;
	D3DProgram _d3dProgram;
	Quadrangle2D _quadrangle;
#endif

	void visitRenderQueue(const std::vector<RenderCommand*> queue);
	void executeRenderCommand(RenderCommand* command);
};

} // namespace mgrrenderer
