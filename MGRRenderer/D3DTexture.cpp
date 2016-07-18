#include "D3DTexture.h"

#if defined(MGRRENDERER_USE_DIRECT3D)
#include "TextureUtility.h"
#include "Image.h"
#include "Director.h" // TODO:Directorに依存してるってどうなんだろ。。
#include <directxtex/include/DirectXTex.h>

namespace mgrrenderer
{

D3DTexture::D3DTexture() :
_depthStencilView(nullptr),
_renderTargetView(nullptr),
_shaderResourceView(nullptr),
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

	if (_shaderResourceView != nullptr)
	{
		_shaderResourceView->Release();
		_shaderResourceView = nullptr;
	}

	if (_renderTargetView != nullptr)
	{
		_renderTargetView->Release();
		_renderTargetView= nullptr;
	}

	if (_depthStencilView != nullptr)
	{
		_depthStencilView->Release();
		_depthStencilView = nullptr;
	}
}

bool D3DTexture::initWithImage(const Image& image, TextureUtility::PixelFormat format)
{
	if (image.getData() == nullptr)
	{
		return false;
	}

	HRESULT result = E_FAIL;
	DirectX::TexMetadata metaData;
	DirectX::ScratchImage scratchImage;
	switch (image.getFileFormat())
	{
	case Image::FileFormat::PNG:
	{
		result = DirectX::LoadFromWICMemory(image.getData(), image.getDataLength(), 0, &metaData, scratchImage);
		if (FAILED(result))
		{
			Logger::logAssert(false, "LoadFromTGAMemory failed. result=%d", result);
			return false;
		}
	}
		break;
	case Image::FileFormat::TGA:
	{
		result = DirectX::LoadFromTGAMemory(image.getData(), image.getDataLength(), &metaData, scratchImage);
		if (FAILED(result))
		{
			Logger::logAssert(false, "LoadFromTGAMemory failed. result=%d", result);
			return false;
		}
	}
		break;
	default:
		break;
	}

	result = CreateShaderResourceView(Director::getInstance()->getDirect3dDevice(), scratchImage.GetImages(), scratchImage.GetImageCount(), metaData, &_shaderResourceView); 
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
	_shaderResourceView->GetResource(&resource);

	D3D11_RESOURCE_DIMENSION type;
	resource->GetType(&type);
	Logger::logAssert(type == D3D11_RESOURCE_DIMENSION_TEXTURE2D, "テクスチャリソースがD3D11_RESOURCE_DIMENSION_TEXTURE2D型でない。type=%d", type);

	D3D11_TEXTURE2D_DESC texDesc;
	static_cast<ID3D11Texture2D*>(resource)->GetDesc(&texDesc);
	_contentSize = Size(texDesc.Width, texDesc.Height);

	return true;
}

bool D3DTexture::initWithTexture(ID3D11ShaderResourceView* shaderResourceView, ID3D11SamplerState* samplerState)
{
	_shaderResourceView = shaderResourceView;
	_samplerState = samplerState;

	// コンテンツサイズ取得
	ID3D11Resource* resource = nullptr;
	_shaderResourceView->GetResource(&resource);

	D3D11_RESOURCE_DIMENSION type;
	resource->GetType(&type);
	Logger::logAssert(type == D3D11_RESOURCE_DIMENSION_TEXTURE2D, "テクスチャリソースがD3D11_RESOURCE_DIMENSION_TEXTURE2D型でない。type=%d", type);

	D3D11_TEXTURE2D_DESC texDesc;
	static_cast<ID3D11Texture2D*>(resource)->GetDesc(&texDesc);
	_contentSize = Size(texDesc.Width, texDesc.Height);

	return true;
}

bool D3DTexture::initDepthStencilTexture(const Size & size, UINT depthStencilViewDescFlags)
{
	ID3D11Device* device = Director::getInstance()->getDirect3dDevice();

	// テクスチャ生成
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = size.width;
	texDesc.Height = size.height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D* texture = nullptr;
	HRESULT result = device->CreateTexture2D(&texDesc, nullptr, &texture);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateTexture2D failed. result=%d", result);
		return false;
	}

	// デプスステンシルビュー生成
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = depthStencilViewDescFlags;
	dsvDesc.Texture2D.MipSlice = 0;

	result = device->CreateDepthStencilView(texture, &dsvDesc, &_depthStencilView);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateDepthStencilView failed. result=%d", result);
		return false;
	}

	// シェーダリソースビュー生成
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(texture, &srvDesc, &_shaderResourceView);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateShaderResourceView failed. result=%d", result);
		return false;
	}

	// サンプラー生成
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 2;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MaxLOD = FLT_MAX;

	result = result = device->CreateSamplerState(&samplerDesc, &_samplerState);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateSamplerState failed. result=%d", result);
		return false;
	}

	return true;
}

bool D3DTexture::initRenderTexture(const Size & size, DXGI_FORMAT textureFormat)
{
	ID3D11Device* device = Director::getInstance()->getDirect3dDevice();

	// テクスチャ生成
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = size.width;
	texDesc.Height = size.height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = textureFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D* texture = nullptr;
	HRESULT result = device->CreateTexture2D(&texDesc, nullptr, &texture);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateTexture2D failed. result=%d", result);
		return false;
	}

	// レンダーターゲットビュー生成
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = textureFormat;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;

	ID3D11RenderTargetView* rtv = nullptr;
	result = device->CreateRenderTargetView(texture, &rtvDesc, &_renderTargetView);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateRenderTargetView failed. result=%d", result);
		return false;
	}

	// シェーダリソースビュー生成
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = textureFormat;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(texture, &srvDesc, &_shaderResourceView);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateShaderResourceView failed. result=%d", result);
		return false;
	}

	// サンプラー生成
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 2;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MaxLOD = FLT_MAX;

	result = result = device->CreateSamplerState(&samplerDesc, &_samplerState);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateSamplerState failed. result=%d", result);
		return false;
	}

	return true;
}

} // namespace mgrrenderer

#endif
