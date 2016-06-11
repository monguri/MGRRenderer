#include "D3DProgram.h"

#if defined(MGRRENDERER_USE_DIRECT3D)
#include "FileUtility.h"
#include "Director.h"
#include <d3dcompiler.h>

namespace mgrrenderer
{
const std::string D3DProgram::SEMANTIC_POSITION = "POSITION";
const std::string D3DProgram::SEMANTIC_COLOR = "COLOR";
const std::string D3DProgram::SEMANTIC_TEXTURE_COORDINATE = "TEX_COORD";
const std::string D3DProgram::SEMANTIC_TEXTURE_COORDINATE_1 = "TEX_COORD1";
const std::string D3DProgram::SEMANTIC_TEXTURE_COORDINATE_2 = "TEX_COORD2";
const std::string D3DProgram::SEMANTIC_TEXTURE_COORDINATE_3 = "TEX_COORD3";
const std::string D3DProgram::SEMANTIC_NORMAL = "NORMAL";
const std::string D3DProgram::SEMANTIC_BLEND_WEIGHT = "BLEND_WEIGHT";
const std::string D3DProgram::SEMANTIC_BLEND_INDEX = "BLEND_INDEX";

D3DProgram::D3DProgram() :
_vertexShader(nullptr),
_vertexShaderBlob(nullptr),
_geometryShader(nullptr),
_pixelShader(nullptr),
_blendState(nullptr),
_rasterizeState(nullptr),
_depthStencilState(nullptr),
_indexBuffer(nullptr),
_inputLayout(nullptr)
{
}

D3DProgram::~D3DProgram()
{
	for (ID3D11Buffer* constantBuffer : _constantBuffers)
	{
		constantBuffer->Release();
	}

	if (_inputLayout != nullptr)
	{
		_inputLayout->Release();
		_inputLayout = nullptr;
	}

	if (_indexBuffer != nullptr)
	{
		_indexBuffer->Release();
		_indexBuffer = nullptr;
	}

	for (ID3D11Buffer* vertexBuffer : _vertexBuffers)
	{
		vertexBuffer->Release();
	}

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

	if (_vertexShader != nullptr)
	{
		_vertexShader->Release();
		_vertexShader = nullptr;
	}

	if (_vertexShaderBlob != nullptr)
	{
		_vertexShaderBlob->Release();
		_vertexShaderBlob = nullptr;
	}
}

void D3DProgram::initWithShaderFile(const std::string & path, bool depthTestEnable, const std::string& vertexShaderFunctionName, const std::string& geometryShaderFunctionName, const std::string& pixelShaderFunctionName)
{
	WCHAR wPath[FileUtility::MAX_PATH_LENGTH] = { 0 };
	FileUtility::convertWCHARFilePath(path, wPath, FileUtility::MAX_PATH_LENGTH);

	ID3DBlob* errMsg = nullptr;
	HRESULT result = E_FAIL;
	ID3D11Device* direct3dDevice = Director::getInstance()->getDirect3dDevice(); // TODO:ここでDirector依存性が生まれるの嫌だなー

	if (!vertexShaderFunctionName.empty())
	{
		// 頂点シェーダのコードをコンパイル
		result = D3DCompileFromFile(
			wPath,
			nullptr,
			nullptr,
			vertexShaderFunctionName.c_str(),
			"vs_4_0",
			D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR,
			0,
			&_vertexShaderBlob,
			&errMsg
		);
		if (FAILED(result))
		{
			Logger::logAssert(false, "D3DX11CompileFromFile failed. result=%d. error message=%s", result, errMsg->GetBufferPointer());
			return;
		}

		// 頂点シェーダの作成
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
	}

	// ジオメトリシェーダのコンパイル
	if (!geometryShaderFunctionName.empty())
	{
		ID3DBlob* blobGS = nullptr;
		result = D3DCompileFromFile(
			wPath,
			nullptr,
			nullptr,
			geometryShaderFunctionName.c_str(),
			"gs_4_0",
			D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR,
			0,
			&blobGS,
			&errMsg
		);
		if (FAILED(result))
		{
			Logger::logAssert(false, "D3DX11CompileFromFile failed. result=%d. error message=%s", result, errMsg->GetBufferPointer());
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
	}

	// ピクセルシェーダのコンパイル
	if (!pixelShaderFunctionName.empty())
	{
		ID3DBlob* blobPS = nullptr;
		result = D3DCompileFromFile(
			wPath,
			nullptr,
			nullptr,
			pixelShaderFunctionName.c_str(),
			"ps_4_0",
			D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR,
			0,
			&blobPS,
			&errMsg
		);
		if (FAILED(result))
		{
			Logger::logAssert(false, "D3DX11CompileFromFile failed. result=%d. error message=%s", result, errMsg->GetBufferPointer());
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

DXGI_FORMAT D3DProgram::getDxgiFormat(const std::string& semantic)
{
	if (semantic == SEMANTIC_POSITION)
	{
		return DXGI_FORMAT_R32G32B32_FLOAT;
	}
	else if (semantic == SEMANTIC_COLOR)
	{
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	}
	else if (semantic == SEMANTIC_TEXTURE_COORDINATE)
	{
		return DXGI_FORMAT_R32G32_FLOAT;
	}
	else if (semantic == SEMANTIC_TEXTURE_COORDINATE_1)
	{
		return DXGI_FORMAT_R32G32_FLOAT;
	}
	else if (semantic == SEMANTIC_TEXTURE_COORDINATE_2)
	{
		return DXGI_FORMAT_R32G32_FLOAT;
	}
	else if (semantic == SEMANTIC_TEXTURE_COORDINATE_3)
	{
		return DXGI_FORMAT_R32G32_FLOAT;
	}
	else if (semantic == SEMANTIC_NORMAL)
	{
		return DXGI_FORMAT_R32G32B32_FLOAT;
	}
	else if (semantic == SEMANTIC_BLEND_WEIGHT)
	{
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	}
	else if (semantic == SEMANTIC_BLEND_INDEX)
	{
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	}
	else
	{
		Logger::logAssert(false, "非対応のセマンティック=%s", semantic.c_str());
		return DXGI_FORMAT_UNKNOWN;
	}
}

void D3DProgram::setShadersToDirect3DContext(ID3D11DeviceContext* context)
{
	context->VSSetShader(_vertexShader, nullptr, 0);
	context->GSSetShader(_geometryShader, nullptr, 0);
	context->PSSetShader(_pixelShader, nullptr, 0);
}

void D3DProgram::setConstantBuffersToDirect3DContext(ID3D11DeviceContext* context)
{
	size_t constantBufferSize = _constantBuffers.size();
	context->VSSetConstantBuffers(0, constantBufferSize, _constantBuffers.data());
	context->GSSetConstantBuffers(0, constantBufferSize, _constantBuffers.data());
	context->PSSetConstantBuffers(0, constantBufferSize, _constantBuffers.data());
}
} // namespace mgrrenderer

#endif
