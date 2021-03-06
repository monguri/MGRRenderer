#pragma once

#include "Config.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include <d3d11.h>
#include "Texture.h"

namespace mgrrenderer
{

class D3DTexture final : public Texture
{
public:
	D3DTexture();
	~D3DTexture();
	bool initWithImage(const Image& image, TextureUtility::PixelFormat format) override;
	bool initDepthStencilTexture(const SizeUint& size);
	bool initDepthStencilCubeMapTexture(unsigned int size);
	bool initRenderTexture(const SizeUint& size, DXGI_FORMAT textureFormat);

	// デプステクスチャやレンダーターゲットなど描画により書き込みするときにのみ使う
	ID3D11DepthStencilView* getDepthStencilView() const { return _depthStencilView; }
	ID3D11RenderTargetView* getRenderTargetView() const { return _renderTargetView; }

	// 参照するときに使う
	ID3D11ShaderResourceView* getShaderResourceView() const { return _shaderResourceView; }

private:
	ID3D11DepthStencilView* _depthStencilView;
	ID3D11RenderTargetView* _renderTargetView;
	ID3D11ShaderResourceView* _shaderResourceView;
};

} // namespace mgrrenderer
#endif
