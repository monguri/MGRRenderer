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
_depthStencilState(nullptr),
_renderTargetView(nullptr),
_shaderResourceView(nullptr)
{
}

D3DTexture::~D3DTexture()
{
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

	if (_depthStencilState != nullptr)
	{
		_depthStencilState->Release();
		_depthStencilState = nullptr;
	}

	if (_depthStencilView != nullptr)
	{
		_depthStencilView->Release();
		_depthStencilView = nullptr;
	}
}

bool D3DTexture::initWithImage(const Image& image, TextureUtility::PixelFormat format)
{
	(void)format; //未使用変数警告抑制

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

bool D3DTexture::initDepthStencilTexture(const Size & size)
{
	_contentSize = size;

	ID3D11Device* device = Director::getInstance()->getDirect3dDevice();

	// テクスチャ生成
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = static_cast<UINT>(size.width);
	texDesc.Height = static_cast<UINT>(size.height);
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	//texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	//texDesc.Format = DXGI_FORMAT_D32_FLOAT;
	texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
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
	//dsvDesc.Format = texDesc.Format;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = 0;
	dsvDesc.Texture2D.MipSlice = 0;

	result = device->CreateDepthStencilView(texture, &dsvDesc, &_depthStencilView);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateDepthStencilView failed. result=%d", result);
		return false;
	}

	// シェーダリソースビュー生成
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	//srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	//srvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(texture, &srvDesc, &_shaderResourceView);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateShaderResourceView failed. result=%d", result);
		return false;
	}

	// 深度、ステンシルステートオブジェクトの作成
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = TRUE;
	depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	result = device->CreateDepthStencilState(&depthStencilDesc, &_depthStencilState);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateDepthStencilState failed. result=%d", result);
		return false;
	}

	return true;
}

bool D3DTexture::initDepthStencilCubeMapTexture(float size)
{
	_contentSize = Size(size, size);

	ID3D11Device* device = Director::getInstance()->getDirect3dDevice();

	// テクスチャ生成
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = static_cast<UINT>(size);
	texDesc.Height = static_cast<UINT>(size);
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	//texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	//texDesc.Format = DXGI_FORMAT_D32_FLOAT;
	texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	ID3D11Texture2D* texture = nullptr;
	HRESULT result = device->CreateTexture2D(&texDesc, nullptr, &texture);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateTexture2D failed. result=%d", result);
		return false;
	}

	// デプスステンシルビュー生成
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	//dsvDesc.Format = texDesc.Format;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Flags = 0;
	dsvDesc.Texture2DArray.MipSlice = 0;
	dsvDesc.Texture2DArray.FirstArraySlice = 0;
	dsvDesc.Texture2DArray.ArraySize = 6;

	result = device->CreateDepthStencilView(texture, &dsvDesc, &_depthStencilView);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateDepthStencilView failed. result=%d", result);
		return false;
	}

	// シェーダリソースビュー生成
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	//srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	//srvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(texture, &srvDesc, &_shaderResourceView);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateShaderResourceView failed. result=%d", result);
		return false;
	}

	// 深度、ステンシルステートオブジェクトの作成
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = TRUE;
	depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	result = device->CreateDepthStencilState(&depthStencilDesc, &_depthStencilState);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateDepthStencilState failed. result=%d", result);
		return false;
	}

	return true;
}

bool D3DTexture::initRenderTexture(const Size & size, DXGI_FORMAT textureFormat)
{
	_contentSize = size;

	ID3D11Device* device = Director::getInstance()->getDirect3dDevice();

	// テクスチャ生成
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = static_cast<UINT>(size.width);
	texDesc.Height = static_cast<UINT>(size.height);
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

	return true;
}

} // namespace mgrrenderer

#endif
