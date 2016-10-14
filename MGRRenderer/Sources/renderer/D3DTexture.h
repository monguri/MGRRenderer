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
	bool initDepthStencilTexture(const Size& size);
	bool initRenderTexture(const Size& size, DXGI_FORMAT textureFormat);

	// �f�v�X�e�N�X�`���⃌���_�[�^�[�Q�b�g�ȂǕ`��ɂ�菑�����݂���Ƃ��ɂ̂ݎg��
	ID3D11DepthStencilView* getDepthStencilView() const { return _depthStencilView; }
	ID3D11DepthStencilState* getDepthStencilState() const { return _depthStencilState; }
	ID3D11RenderTargetView* getRenderTargetView() const { return _renderTargetView; }

	// �Q�Ƃ���Ƃ��Ɏg��
	ID3D11ShaderResourceView* getShaderResourceView() const { return _shaderResourceView; }
	ID3D11SamplerState* getSamplerState() const { return _samplerState; }

private:
	ID3D11DepthStencilView* _depthStencilView;
	ID3D11DepthStencilState* _depthStencilState;
	ID3D11RenderTargetView* _renderTargetView;
	ID3D11ShaderResourceView* _shaderResourceView;
	ID3D11SamplerState* _samplerState;
};

} // namespace mgrrenderer
#endif
