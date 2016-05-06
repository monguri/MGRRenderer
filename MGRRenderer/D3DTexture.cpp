#include "D3DTexture.h"

#if defined(MGRRENDERER_USE_DIRECT3D)
#include "TextureUtility.h"
#include "Image.h"
#include "Director.h" // TODO:Directorに依存してるってどうなんだろ。。

namespace mgrrenderer
{

D3DTexture::D3DTexture() :
_resourceView(nullptr),
_samplerState(nullptr)
{
}

D3DTexture::~D3DTexture()
{
	if (_samplerState != nullptr)
	{
		_samplerState->Release();
		_samplerState = nullptr;
	}

	if (_resourceView != nullptr)
	{
		_resourceView->Release();
		_resourceView = nullptr;
	}
}

bool D3DTexture::initWithImage(const Image& image, TextureUtility::PixelFormat format)
{
	if (image.getData() == nullptr)
	{
		return false;
	}

	//ID3D11Resource* texture = nullptr;
	//HRESULT result = D3DX11CreateTextureFromFile(Director::getInstance()->getDirect3dDevice(), L"..\\Resources\\monguger.tga", nullptr, nullptr, &texture, nullptr);
	//if (FAILED(result))
	//{
	//	Logger::logAssert(false, "D3DX11CreateTextureFromFile failed. result=%d", result);
	//	return false;
	//}

	//D3D11_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
	//resourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//resourceViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	//resourceViewDesc.Texture2D.MostDetailedMip = 0;
	//resourceViewDesc.Texture2D.MipLevels = -1;

	//result = Director::getInstance()->getDirect3dDevice()->CreateShaderResourceView(texture, nullptr, &_resourceView);
	HRESULT result = D3DX11CreateShaderResourceViewFromMemory(Director::getInstance()->getDirect3dDevice(), (LPCVOID)image.getData(), (SIZE_T)image.getDataLength(), nullptr, nullptr, &_resourceView, nullptr);
	if (FAILED(result))
	{
		Logger::logAssert(false, "D3DX11CreateShaderResourceViewFromMemory failed. result=%d", result);
		return false;
	}

	D3D11_SAMPLER_DESC desc;
	desc.Filter = D3D11_FILTER_ANISOTROPIC;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = 2;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	desc.BorderColor[0] = 0.0f;
	desc.BorderColor[1] = 0.0f;
	desc.BorderColor[2] = 0.0f;
	desc.BorderColor[3] = 0.0f;
	desc.MinLOD = -FLT_MAX;
	desc.MaxLOD = FLT_MAX;
	result = Director::getInstance()->getDirect3dDevice()->CreateSamplerState(&desc, &_samplerState);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateSamplerState failed. result=%d", result);
		return false;
	}

	// コンテンツサイズ取得
	ID3D11Resource* resource = nullptr;
	_resourceView->GetResource(&resource);

	D3D11_RESOURCE_DIMENSION type;
	resource->GetType(&type);
	Logger::logAssert(type == D3D11_RESOURCE_DIMENSION_TEXTURE2D, "テクスチャリソースがD3D11_RESOURCE_DIMENSION_TEXTURE2D型でない。type=%d", type);

	D3D11_TEXTURE2D_DESC texDesc;
	static_cast<ID3D11Texture2D*>(resource)->GetDesc(&texDesc);
	_contentSize = Size(texDesc.Width, texDesc.Height);

	return true;
}

} // namespace mgrrenderer

#endif
