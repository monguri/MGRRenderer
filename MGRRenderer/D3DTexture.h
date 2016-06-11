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
	bool initWithTexture(ID3D11ShaderResourceView* shaderResourceView, ID3D11SamplerState* samplerState);
	ID3D11ShaderResourceView* getShaderResourceView() const { return _resourceView; }
	ID3D11SamplerState* getSamplerState() const { return _samplerState; }

private:
	ID3D11ShaderResourceView* _resourceView;
	ID3D11SamplerState* _samplerState;
};

} // namespace mgrrenderer
#endif
