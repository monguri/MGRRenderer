#include "D3DProgram.h"

#if defined(MGRRENDERER_USE_DIRECT3D)
#include "FileUtility.h"
#include "Director.h"

namespace mgrrenderer
{
D3DProgram::D3DProgram() :
_vertexShader(nullptr),
_vertexShaderBlob(nullptr),
_geometryShader(nullptr),
_pixelShader(nullptr),
_blendState(nullptr),
_rasterizeState(nullptr),
_depthStencilState(nullptr)
{
}

D3DProgram::~D3DProgram()
{
	if (_depthStencilState != nullptr)
	{
		_depthStencilState->Release();
		_depthStencilState = nullptr;
	}

	if (_rasterizeState != nullptr)
	{
		_rasterizeState->Release();
		_rasterizeState = nullptr;
	}

	if (_blendState != nullptr)
	{
		_blendState->Release();
		_blendState = nullptr;
	}

	if (_pixelShader != nullptr)
	{
		_pixelShader->Release();
		_pixelShader = nullptr;
	}

	if (_geometryShader != nullptr)
	{
		_geometryShader->Release();
		_geometryShader = nullptr;
	}

	if (_vertexShaderBlob != nullptr)
	{
		_vertexShaderBlob->Release();
		_vertexShaderBlob = nullptr;
	}

	if (_vertexShader != nullptr)
	{
		_vertexShader->Release();
		_vertexShader = nullptr;
	}
}

void D3DProgram::initWithShaderFile(const std::string & path, bool depthTestEnable)
{
	WCHAR wPath[FileUtility::MAX_PATH_LENGTH] = { 0 };
	FileUtility::convertWCHARFilePath(path, wPath, FileUtility::MAX_PATH_LENGTH);

	// 頂点シェーダのコードをコンパイル
	HRESULT result = D3DX11CompileFromFile(
		wPath,
		nullptr,
		nullptr,
		"VS",
		"vs_4_0",
		D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR,
		0,
		nullptr,
		&_vertexShaderBlob,
		nullptr,
		nullptr
	);
	if (FAILED(result))
	{
		Logger::logAssert(false, "D3DX11CompileFromFile failed. result=%d", result);
		return;
	}

	// 頂点シェーダの作成
	ID3D11Device* direct3dDevice = Director::getInstance()->getDirect3dDevice(); // TODO:ここでDirector依存性が生まれるの嫌だなー
	result = direct3dDevice->CreateVertexShader(
		_vertexShaderBlob->GetBufferPointer(),
		_vertexShaderBlob->GetBufferSize(),
		nullptr,
		&_vertexShader
	);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateVertexShader failed. result=%d", result);
		return;
	}

	// ジオメトリシェーダのコンパイル
	ID3DBlob* blobGS = nullptr;
	result = D3DX11CompileFromFile(
		wPath,
		nullptr,
		nullptr,
		"GS",
		"gs_4_0",
		D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR,
		0,
		nullptr,
		&blobGS,
		nullptr,
		nullptr
	);
	if (FAILED(result))
	{
		Logger::logAssert(false, "D3DX11CompileFromFile failed. result=%d", result);
		return;
	}

	// ジオメトリシェーダの作成
	result = direct3dDevice->CreateGeometryShader(
		blobGS->GetBufferPointer(),
		blobGS->GetBufferSize(),
		nullptr,
		&_geometryShader
	);
	blobGS->Release();
	blobGS = nullptr;
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateVertexShader failed. result=%d", result);
		return;
	}

	// ピクセルシェーダのコンパイル
	ID3DBlob* blobPS = nullptr;
	result = D3DX11CompileFromFile(
		wPath,
		nullptr,
		nullptr,
		"PS",
		"ps_4_0",
		D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR,
		0,
		nullptr,
		&blobPS,
		nullptr,
		nullptr
	);
	if (FAILED(result))
	{
		Logger::logAssert(false, "D3DX11CompileFromFile failed. result=%d", result);
		return;
	}

	// ピクセルシェーダの作成
	result = direct3dDevice->CreatePixelShader(
		blobPS->GetBufferPointer(),
		blobPS->GetBufferSize(),
		nullptr,
		&_pixelShader
	);
	blobPS->Release();
	blobPS = nullptr;
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreatePixelShader failed. result=%d", result);
		return;
	}

	// ブレンドステートオブジェクトの作成
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	result = direct3dDevice->CreateBlendState(&blendDesc, &_blendState);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBlendState failed. result=%d", result);
		return;
	}

	// ラスタライズステートオブジェクトの作成
	D3D11_RASTERIZER_DESC rasterizeDesc;
	rasterizeDesc.FillMode = D3D11_FILL_SOLID;
	rasterizeDesc.CullMode = D3D11_CULL_NONE;
	rasterizeDesc.FrontCounterClockwise = FALSE;
	rasterizeDesc.DepthBias = 0;
	rasterizeDesc.DepthBiasClamp = 0;
	rasterizeDesc.SlopeScaledDepthBias = 0;
	rasterizeDesc.DepthClipEnable = TRUE;
	rasterizeDesc.ScissorEnable = FALSE;
	rasterizeDesc.MultisampleEnable = FALSE;
	rasterizeDesc.AntialiasedLineEnable = FALSE;
	result = direct3dDevice->CreateRasterizerState(&rasterizeDesc, &_rasterizeState);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateRasterizerState failed. result=%d", result);
		return;
	}

	// 深度、ステンシルステートオブジェクトの作成
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = (depthTestEnable ? TRUE : FALSE);
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0;
	depthStencilDesc.StencilWriteMask = 0;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	result = direct3dDevice->CreateDepthStencilState(&depthStencilDesc, &_depthStencilState);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateDepthStencilState failed. result=%d", result);
		return;
	}
}

} // namespace mgrrenderer

#endif
