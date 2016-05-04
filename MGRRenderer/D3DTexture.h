#pragma once

#include "Config.h"
#if defined(MGRRENDERER_USE_DIRECT3D)

#include "Texture.h"
#include <d3dx11.h>

namespace mgrrenderer
{

class D3DTexture final : public Texture
{
public:
	D3DTexture();
	~D3DTexture();
	bool initWithImage(const Image& image, TextureUtility::PixelFormat format) override;
	ID3D11ShaderResourceView* getShaderResourceView() const { return _resourceView; }
	ID3D11SamplerState* getSamplerState() const { return _samplerState; }

private:
	ID3D11ShaderResourceView* _resourceView;
	ID3D11SamplerState* _samplerState;
};

} // namespace mgrrenderer
#endif
