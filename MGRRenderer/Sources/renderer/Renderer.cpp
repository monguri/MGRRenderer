#include "Renderer.h"
#include "Director.h"
#include "RenderCommand.h"
#include "GroupBeginRenderCommand.h"
#include "utility/Logger.h"
#include "node/Light.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DTexture.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "GLFrameBuffer.h"
#include "GLTexture.h"
#include "Shaders.h"
#endif

namespace mgrrenderer
{

static const size_t DEFAULT_RENDER_QUEUE_GROUP_INDEX = 0;

Renderer::Renderer() :
_drawWireFrame(false)
,_renderMode(RenderMode::LIGHTING)
#if defined(MGRRENDERER_USE_DIRECT3D)
,_direct3dSwapChain(nullptr)
,_direct3dDevice(nullptr)
,_direct3dContext(nullptr)
,_direct3dRenderTarget(nullptr)
,_direct3dDepthStencilView(nullptr)
,_direct3dDepthStencilState(nullptr)
,_direct3dDepthStencilStateTransparent(nullptr)
,_direct3dDepthStencilState2D(nullptr)
,_pointSampler(nullptr)
,_linearSampler(nullptr)
,_pcfSampler(nullptr)
,_rasterizeStateNormal(nullptr)
,_rasterizeStateWireFrame(nullptr)
,_blendState(nullptr)
,_blendStateTransparent(nullptr)
#endif
#if defined(MGRRENDERER_DEFERRED_RENDERING)
#if defined(MGRRENDERER_USE_DIRECT3D)
,_gBufferDepthStencil(nullptr)
,_gBufferColorSpecularIntensity(nullptr)
,_gBufferNormal(nullptr)
,_gBufferSpecularPower(nullptr)
#elif defined(MGRRENDERER_USE_OPENGL)
,_gBufferFrameBuffer(nullptr)
#endif
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)
{
	_groupIndexStack.push(DEFAULT_RENDER_QUEUE_GROUP_INDEX);

	std::vector<RenderCommand*> defaultRenderQueue;
	_queueGroup.push_back(defaultRenderQueue);
}

Renderer::~Renderer()
{
#if defined(MGRRENDERER_DEFERRED_RENDERING)
#if defined(MGRRENDERER_USE_DIRECT3D)
	if (_blendStateTransparent != nullptr)
	{
		_blendStateTransparent->Release();
		_blendStateTransparent = nullptr;
	}

	if (_blendState != nullptr)
	{
		_blendState->Release();
		_blendState = nullptr;
	}

	if (_gBufferSpecularPower != nullptr)
	{
		delete _gBufferSpecularPower;
		_gBufferSpecularPower = nullptr;
	}

	if (_gBufferNormal != nullptr)
	{
		delete _gBufferNormal;
		_gBufferNormal = nullptr;
	}

	if (_gBufferColorSpecularIntensity != nullptr)
	{
		delete _gBufferColorSpecularIntensity;
		_gBufferColorSpecularIntensity = nullptr;
	}

	if (_gBufferDepthStencil != nullptr)
	{
		delete _gBufferDepthStencil;
		_gBufferDepthStencil = nullptr;
	}

	if (_direct3dDepthStencilState2D != nullptr)
	{
		_direct3dDepthStencilState2D->Release();
		_direct3dDepthStencilState2D = nullptr;
	}

	if (_direct3dDepthStencilStateTransparent != nullptr)
	{
		_direct3dDepthStencilStateTransparent ->Release();
		_direct3dDepthStencilStateTransparent  = nullptr;
	}

	if (_direct3dDepthStencilState != nullptr)
	{
		_direct3dDepthStencilState->Release();
		_direct3dDepthStencilState = nullptr;
	}

	if (_direct3dDepthStencilView != nullptr)
	{
		_direct3dDepthStencilView->Release();
		_direct3dDepthStencilView = nullptr;
	}

	if (_direct3dRenderTarget != nullptr)
	{
		_direct3dRenderTarget->Release();
		_direct3dRenderTarget = nullptr;
	}

	if (_direct3dContext != nullptr)
	{
		_direct3dContext->Release();
		_direct3dContext = nullptr;
	}

	if (_direct3dDevice != nullptr)
	{
		_direct3dDevice->Release();
		_direct3dDevice = nullptr;
	}

	if (_direct3dSwapChain != nullptr)
	{
		_direct3dSwapChain->Release();
		_direct3dSwapChain = nullptr;
	}
#elif defined(MGRRENDERER_USE_OPENGL)
	if (_gBufferFrameBuffer != nullptr)
	{
		delete _gBufferFrameBuffer;
		_gBufferFrameBuffer = nullptr;
	}
#endif
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

#if defined(MGRRENDERER_USE_DIRECT3D)
	if (_rasterizeStateWireFrame != nullptr)
	{
		_rasterizeStateWireFrame ->Release();
		_rasterizeStateWireFrame  = nullptr;
	}

	if (_rasterizeStateNormal != nullptr)
	{
		_rasterizeStateNormal->Release();
		_rasterizeStateNormal = nullptr;
	}

	if (_pcfSampler != nullptr)
	{
		_pcfSampler->Release();
		_pcfSampler = nullptr;
	}

	if (_linearSampler != nullptr)
	{
		_linearSampler->Release();
		_linearSampler = nullptr;
	}

	if (_pointSampler != nullptr)
	{
		_pointSampler->Release();
		_pointSampler = nullptr;
	}
#endif
}

#if defined(MGRRENDERER_USE_DIRECT3D)
void Renderer::initView(HWND handleWindow, const SizeUint& windowSize)
#elif defined(MGRRENDERER_USE_OPENGL)
void Renderer::initView(const SizeUint& windowSize)
#endif
{
	(void)windowSize;

#if defined(MGRRENDERER_USE_DIRECT3D)
	// デバイスとスワップ チェインの作成
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BufferCount = 1;
	desc.BufferDesc.Width = (UINT)windowSize.width;
	desc.BufferDesc.Height = (UINT)windowSize.height;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.RefreshRate.Numerator = 60;
	desc.BufferDesc.RefreshRate.Denominator = 60;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = handleWindow;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Windowed = TRUE;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// ハードウェア・デバイスを作成
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	D3D_FEATURE_LEVEL featureLevelSupported;

	D3D_DRIVER_TYPE driverTypes[] = { D3D_DRIVER_TYPE_HARDWARE, // ハードウェア・デバイス
									D3D_DRIVER_TYPE_WARP, // WARPデバイス
									D3D_DRIVER_TYPE_REFERENCE }; // リファレンス・デバイスを作成

	HRESULT result = 0; //TODO:定数にしたい
	for (D3D_DRIVER_TYPE driverType : driverTypes)
	{
		result = D3D11CreateDeviceAndSwapChain(
			nullptr,
			driverType,
			nullptr,
			D3D11_CREATE_DEVICE_DEBUG, // デフォルトでデバッグにしておく
			featureLevels,
			3,
			D3D11_SDK_VERSION,
			&desc,
			&_direct3dSwapChain,
			&_direct3dDevice,
			&featureLevelSupported,
			&_direct3dContext
		);

		if (SUCCEEDED(result))
		{
			break;
		}
	}

	if (FAILED(result))
	{
		Logger::logAssert(false, "Error:%s D3D11CreateDeviceAndSwapChain failed.", GetLastError());
		// TODO:それぞれのエラー処理で解放処理をちゃんと書かないと
		return;
	}

	// スワップ・チェインから最初のバック・バッファを取得する
	ID3D11Texture2D* backBuffer = nullptr;
	result = _direct3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "Error:%s GetBuffer failed.", GetLastError());
		return;
	}

	// バック・バッファの描画ターゲット・ビューを作る
	result = _direct3dDevice->CreateRenderTargetView(backBuffer, nullptr, &_direct3dRenderTarget);
	if (FAILED(result))
	{
		Logger::logAssert(false, "Error:%s CreateRenderTargetView failed.", GetLastError());
		return;
	}

	// バック・バッファの情報
	D3D11_TEXTURE2D_DESC descBackBuffer;
	backBuffer->GetDesc(&descBackBuffer);

	// 深度/ステンシル・テクスチャの作成
	D3D11_TEXTURE2D_DESC descDepthStencilTexture = descBackBuffer;
	descDepthStencilTexture.MipLevels = 1;
	descDepthStencilTexture.ArraySize = 1;
	descDepthStencilTexture.Format = DXGI_FORMAT_D32_FLOAT;
	descDepthStencilTexture.Usage = D3D11_USAGE_DEFAULT;
	descDepthStencilTexture.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepthStencilTexture.CPUAccessFlags = 0;
	descDepthStencilTexture.MiscFlags = 0;

	ID3D11Texture2D* depthStencilTexture = nullptr;
	result = _direct3dDevice->CreateTexture2D(&descDepthStencilTexture, nullptr, &depthStencilTexture);
	if (FAILED(result))
	{
		Logger::logAssert(false, "Error:%s CreateTexture2D failed.", GetLastError());
		return;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC descDepthStencilView;
	descDepthStencilView.Format = descDepthStencilTexture.Format;
	descDepthStencilView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDepthStencilView.Flags = 0;
	descDepthStencilView.Texture2D.MipSlice = 0;

	result = _direct3dDevice->CreateDepthStencilView(depthStencilTexture, &descDepthStencilView, &_direct3dDepthStencilView);
	if (FAILED(result))
	{
		Logger::logAssert(false, "Error:%s CreateDepthStencilView failed.", GetLastError());
		return;
	}

	//
	// 深度、ステンシルステートオブジェクトの作成
	//

	// 通常のデプステスト用
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	result = _direct3dDevice->CreateDepthStencilState(&depthStencilDesc, &_direct3dDepthStencilState);
	if (FAILED(result))
	{
		Logger::logAssert(false, "Error:%s CreateDepthStencilState failed.", GetLastError());
		return;
	}

	// デプステストはするがデプスバッファには書き込まない透過オブジェクトパス用
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	result = _direct3dDevice->CreateDepthStencilState(&depthStencilDesc, &_direct3dDepthStencilStateTransparent);
	if (FAILED(result))
	{
		Logger::logAssert(false, "Error:%s CreateDepthStencilState failed.", GetLastError());
		return;
	}

	// デプステストをしない2D用
	depthStencilDesc.DepthEnable = FALSE;
	result = _direct3dDevice->CreateDepthStencilState(&depthStencilDesc, &_direct3dDepthStencilState2D);
	if (FAILED(result))
	{
		Logger::logAssert(false, "Error:%s CreateDepthStencilState failed.", GetLastError());
		return;
	}

	// ビューポートの設定
	_direct3dViewport[0].TopLeftX = 0.0f;
	_direct3dViewport[0].TopLeftY = 0.0f;
	_direct3dViewport[0].Width = static_cast<FLOAT>(windowSize.width);
	_direct3dViewport[0].Height = static_cast<FLOAT>(windowSize.height);
	_direct3dViewport[0].MinDepth = 0.0f;
	_direct3dViewport[0].MaxDepth = 1.0f;
	_direct3dContext->RSSetViewports(1, _direct3dViewport);

	// 汎用サンプラの用意
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;
	samplerDesc.MinLOD = -FLT_MAX;
	samplerDesc.MaxLOD = FLT_MAX;
	result = _direct3dDevice->CreateSamplerState(&samplerDesc, &_pointSampler);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateSamplerState failed. result=%d", result);
		return;
	}

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	result = _direct3dDevice->CreateSamplerState(&samplerDesc, &_linearSampler);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateSamplerState failed. result=%d", result);
		return;
	}

	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	result = _direct3dDevice->CreateSamplerState(&samplerDesc, &_pcfSampler);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateSamplerState failed. result=%d", result);
		return;
	}

	// 汎用ラスタライズステートオブジェクトの作成　フェイスカリングなし
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
	result = _direct3dDevice->CreateRasterizerState(&rasterizeDesc, &_rasterizeStateNormal);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateRasterizerState failed. result=%d", result);
		return;
	}

	rasterizeDesc.FillMode = D3D11_FILL_WIREFRAME;
	result = _direct3dDevice->CreateRasterizerState(&rasterizeDesc, &_rasterizeStateWireFrame);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateRasterizerState failed. result=%d", result);
		return;
	}

	//
	// 汎用ブレンドステートオブジェクトの作成
	//

	// 不透過パス用。ブレンドしない
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	result = _direct3dDevice->CreateBlendState(&blendDesc, &_blendState);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBlendState failed. result=%d", result);
		return;
	}

	// 透過パス用。ブレンドする
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	result = _direct3dDevice->CreateBlendState(&blendDesc, &_blendStateTransparent);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBlendState failed. result=%d", result);
		return;
	}
#elif defined(MGRRENDERER_USE_OPENGL)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); // デフォルト値は一番奥にしておく
	// OpenGL側でやるビューポート変換のためのパラメータを渡す
	glViewport(0, 0, static_cast<GLsizei>(windowSize.width), static_cast<GLsizei>(windowSize.height));
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // デフォルトのフレームバッファ
#endif

#if defined(MGRRENDERER_DEFERRED_RENDERING)
#if defined(MGRRENDERER_USE_DIRECT3D)
	// Gバッファの準備
	_gBufferDepthStencil = new D3DTexture();
	_gBufferDepthStencil->initDepthStencilTexture(windowSize);

	_gBufferColorSpecularIntensity = new D3DTexture();
	_gBufferColorSpecularIntensity->initRenderTexture(windowSize, DXGI_FORMAT_R8G8B8A8_UNORM);

	_gBufferNormal = new D3DTexture();
	_gBufferNormal->initRenderTexture(windowSize, DXGI_FORMAT_R11G11B10_FLOAT);

	_gBufferSpecularPower = new D3DTexture();
	_gBufferSpecularPower->initRenderTexture(windowSize, DXGI_FORMAT_R8G8B8A8_UNORM);

	//
	// ディファードレンダリングの準備
	//

	_d3dProgramForDeferredRendering.initWithShaderFile("Resources/shader/DeferredLighting.hlsl", true, "VS", "", "PS");

	// 定数バッファの作成
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	constantBufferDesc.ByteWidth = sizeof(Mat4);

	ID3D11Buffer* constantBuffer = nullptr;

	// render mode用
	result = _direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgramForDeferredRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_RENDER_MODE, constantBuffer);

	// View行列用
	constantBuffer = nullptr;
	result = _direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgramForDeferredRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);

	// Projection行列用
	constantBuffer = nullptr;
	result = _direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgramForDeferredRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);

	// デプスバイアス行列用
	constantBuffer = nullptr;
	result = _direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgramForDeferredRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX, constantBuffer);

	// アンビエントライトカラー
	constantBufferDesc.ByteWidth = sizeof(AmbientLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = _direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgramForDeferredRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer);

	// ディレクショナルトライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(DirectionalLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = _direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgramForDeferredRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer);

	// ポイントライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(PointLight::ConstantBufferData) * PointLight::MAX_NUM;
	constantBuffer = nullptr;
	result = _direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgramForDeferredRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer);

	// スポットライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(SpotLight::ConstantBufferData) * SpotLight::MAX_NUM;
	constantBuffer = nullptr;
	result = _direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgramForDeferredRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER, constantBuffer);

	_quadrangle.bottomLeft.position = Vec2(-1.0f, -1.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.bottomRight.position = Vec2(1.0, -1.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0, 1.0f);
	_quadrangle.topLeft.position = Vec2(-1.0f, 1.0f);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.topRight.position = Vec2(1.0, 1.0);
	_quadrangle.topRight.textureCoordinate = Vec2(1.0f, 0.0f);

	// 頂点バッファの定義
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(_quadrangle);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// 頂点バッファのサブリソースの定義
	D3D11_SUBRESOURCE_DATA vertexBufferSubData;
	vertexBufferSubData.pSysMem = &_quadrangle;
	vertexBufferSubData.SysMemPitch = 0;
	vertexBufferSubData.SysMemSlicePitch = 0;

	// 頂点バッファのサブリソースの作成
	ID3D11Buffer* vertexBuffer = nullptr;
	result = _direct3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubData, &vertexBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	std::vector<ID3D11Buffer*> oneMeshVBs;
	oneMeshVBs.push_back(vertexBuffer);
	_d3dProgramForDeferredRendering.addVertexBuffers(oneMeshVBs);

	// 入力レイアウトオブジェクトの作成
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{D3DProgram::SEMANTIC_POSITION.c_str(), 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{D3DProgram::SEMANTIC_TEXTURE_COORDINATE.c_str(), 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(Vec2), D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	ID3D11InputLayout* inputLayout = nullptr;
	result = _direct3dDevice->CreateInputLayout(
		layout,
		_countof(layout), 
		_d3dProgramForDeferredRendering.getVertexShaderBlob()->GetBufferPointer(),
		_d3dProgramForDeferredRendering.getVertexShaderBlob()->GetBufferSize(),
		&inputLayout
	);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateInputLayout failed. result=%d", result);
		return;
	}
	_d3dProgramForDeferredRendering.setInputLayout(inputLayout);
#elif defined(MGRRENDERER_USE_OPENGL)
	// Gバッファの準備
	_gBufferFrameBuffer = new GLFrameBuffer();
	std::vector<GLenum> drawBuffers;
	drawBuffers.push_back(GL_NONE);
	drawBuffers.push_back(GL_COLOR_ATTACHMENT0);
	drawBuffers.push_back(GL_COLOR_ATTACHMENT1);
	drawBuffers.push_back(GL_COLOR_ATTACHMENT2);
	std::vector<GLenum> pixelFormats;
	pixelFormats.push_back(GL_DEPTH_COMPONENT);
	pixelFormats.push_back(GL_RGBA);
	pixelFormats.push_back(GL_RGBA);
	pixelFormats.push_back(GL_RGBA);
	_gBufferFrameBuffer->initWithTextureParams(drawBuffers, pixelFormats, false, false, windowSize);


	//
	// ディファードレンダリングの準備
	//
	_glProgramForDeferredRendering.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderDeferredLighting.glsl", "../MGRRenderer/Resources/shader/FragmentShaderDeferredLighting.glsl");

	_quadrangle.bottomLeft.position = Vec2(-1.0f, -1.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.bottomRight.position = Vec2(1.0, -1.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0, 0.0f);
	_quadrangle.topLeft.position = Vec2(-1.0f, 1.0f);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.topRight.position = Vec2(1.0, 1.0);
	_quadrangle.topRight.textureCoordinate = Vec2(1.0f, 1.0f);
#endif
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)
}

void Renderer::addCommand(RenderCommand* command)
{
	switch (command->getType())
	{
	case RenderCommand::Type::GROUP_BEGIN:
		{
			// スタックのトップのインデックスのキューに追加
			_queueGroup[_groupIndexStack.top()].push_back(command);

			// スタックにプッシュ
			size_t groupIndex = _queueGroup.size();
			GroupBeginRenderCommand* groupCommand = static_cast<GroupBeginRenderCommand*>(command);
			groupCommand->setGroupIndex(groupIndex);
			_groupIndexStack.push(groupIndex);

			std::vector<RenderCommand*> newRenderQueue;
			_queueGroup.push_back(newRenderQueue);
		}
		break;
	case RenderCommand::Type::GROUP_END:
		// addCommandでは、現在操作中のグループIDを知るためにスタックからは削除するが、キューグループからは削除しない。
		_groupIndexStack.pop();
		_queueGroup[_groupIndexStack.top()].push_back(command);
		break;
	case RenderCommand::Type::CUSTOM:
		// スタックのトップのキューに追加
		_queueGroup[_groupIndexStack.top()].push_back(command);
		break;
	default:
		Logger::logAssert(false, "対応していないコマンドタイプが入力された。");
		break;
	}
}

void Renderer::render()
{
	visitRenderQueue(_queueGroup[DEFAULT_RENDER_QUEUE_GROUP_INDEX]);

	Logger::logAssert(_groupIndexStack.size() == 1, "グループコマンド開始側で作られたインデックススタックは終了側で消されてるはず。_groupIndexStack.size() == %d", _queueGroup.size());

	// 0番目のキューを残してあとは削除。次フレームのグループコマンドでまた追加する。削除するのは、シーンの状況でグループ数は変わりうるので、増えたままで残しておいても無駄だから。
	while (_queueGroup.size() > 1)
	{
		_queueGroup.pop_back();
	}

	_queueGroup[DEFAULT_RENDER_QUEUE_GROUP_INDEX].clear();
}

void Renderer::prepareDefaultRenderTarget()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	_direct3dContext->ClearState();

	float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	_direct3dContext->ClearRenderTargetView(_direct3dRenderTarget, clearColor);
	_direct3dContext->ClearDepthStencilView(_direct3dDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	_direct3dContext->RSSetViewports(1, _direct3dViewport);
	_direct3dContext->RSSetState(_drawWireFrame ? _rasterizeStateWireFrame : _rasterizeStateNormal);

	_direct3dContext->OMSetRenderTargets(1, &_direct3dRenderTarget, _direct3dDepthStencilView);
	_direct3dContext->OMSetDepthStencilState(_direct3dDepthStencilState, 1);

	FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	_direct3dContext->OMSetBlendState(_blendState, blendFactor, 0xffffffff);
#elif defined(MGRRENDERER_USE_OPENGL)
	glPolygonMode(GL_FRONT_AND_BACK, _drawWireFrame ? GL_LINE : GL_FILL);
	glLineWidth(2.0f);
	//glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glViewport(0, 0, static_cast<GLsizei>(Director::getInstance()->getWindowSize().width), static_cast<GLsizei>(Director::getInstance()->getWindowSize().height));
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // デフォルトフレームバッファに戻す
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

#if defined(MGRRENDERER_DEFERRED_RENDERING)
void Renderer::prepareGBufferRendering()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	_direct3dContext->ClearState();

	float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	_direct3dContext->ClearRenderTargetView(_gBufferColorSpecularIntensity->getRenderTargetView(), clearColor);
	_direct3dContext->ClearRenderTargetView(_gBufferNormal->getRenderTargetView(), clearColor);
	_direct3dContext->ClearRenderTargetView(_gBufferSpecularPower->getRenderTargetView(), clearColor);
	_direct3dContext->ClearDepthStencilView(_gBufferDepthStencil->getDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	_direct3dContext->RSSetViewports(1, _direct3dViewport);
	_direct3dContext->RSSetState(_drawWireFrame ? _rasterizeStateWireFrame : _rasterizeStateNormal);

	ID3D11RenderTargetView* gBuffers[3] = {_gBufferColorSpecularIntensity->getRenderTargetView(), _gBufferNormal->getRenderTargetView(), _gBufferSpecularPower->getRenderTargetView()};
	_direct3dContext->OMSetRenderTargets(3, gBuffers, _gBufferDepthStencil->getDepthStencilView());
	_direct3dContext->OMSetDepthStencilState(_direct3dDepthStencilState, 1);

	FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	_direct3dContext->OMSetBlendState(_blendState, blendFactor, 0xffffffff);
#elif defined(MGRRENDERER_USE_OPENGL)
	glPolygonMode(GL_FRONT_AND_BACK, _drawWireFrame ? GL_LINE : GL_FILL);
	glLineWidth(2.0f);
	glBindFramebuffer(GL_FRAMEBUFFER, _gBufferFrameBuffer->getFrameBufferId());

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Gバッファの大きさは画面サイズと同じにしている
	glViewport(0, 0, static_cast<GLsizei>(Director::getInstance()->getWindowSize().width), static_cast<GLsizei>(Director::getInstance()->getWindowSize().height));

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND); // Gバッファ描画中は不透過しかあつかわないのでブレンドしない
#endif
}

void Renderer::prepareDeferredRendering()
{
	prepareDefaultRenderTarget();
}

void Renderer::renderDeferred()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();

	// TODO:ここらへん共通化したいな。。
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// render modeのマップ
	HRESULT result = direct3dContext->Map(
		_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_RENDER_MODE),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
	CopyMemory(mappedResource.pData, &_renderMode, sizeof(_renderMode));
	direct3dContext->Unmap(_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_RENDER_MODE), 0);

	// ビュー行列の逆行列のマップ
	result = direct3dContext->Map(
		_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
	Mat4 viewMatrix = Director::getCamera().getViewMatrix().createInverse().transpose();
	CopyMemory(mappedResource.pData, &viewMatrix.m, sizeof(viewMatrix));
	direct3dContext->Unmap(_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX), 0);

	// プロジェクション行列の逆行列のマップ
	result = direct3dContext->Map(
		_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
	Mat4 projectionMatrix = (Mat4::CHIRARITY_CONVERTER * Director::getCamera().getProjectionMatrix()).transpose();
	CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
	direct3dContext->Unmap(_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

	result = direct3dContext->Map(
		_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
	Mat4 depthBiasMatrix = (Mat4::TEXTURE_COORDINATE_CONVERTER * Mat4::createScale(Vec3(0.5f, 0.5f, 1.0f)) * Mat4::createTranslation(Vec3(1.0f, -1.0f, 0.0f))).transpose(); //TODO: Mat4を参照型にすると値がおかしくなってしまう
	CopyMemory(mappedResource.pData, &depthBiasMatrix.m, sizeof(depthBiasMatrix));
	direct3dContext->Unmap(_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX), 0);

	// TODO:こういう風に一個ではなくなる
	const Scene& scene = Director::getInstance()->getScene();

	const AmbientLight* ambientLight = scene.getAmbientLight();
	Logger::logAssert(ambientLight != nullptr, "シーンにアンビエントライトがない。");

	// アンビエントライトカラーのマップ
	result = direct3dContext->Map(
		_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
	CopyMemory(mappedResource.pData, ambientLight->getConstantBufferDataPointer(), sizeof(AmbientLight::ConstantBufferData));
	direct3dContext->Unmap(_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER), 0);

	ID3D11ShaderResourceView* dirLightShadowMapResourceView = nullptr;
	const DirectionalLight* directionalLight = scene.getDirectionalLight();
	if (directionalLight != nullptr)
	{
		if (directionalLight->hasShadowMap())
		{
			dirLightShadowMapResourceView = directionalLight->getShadowMapData().depthTexture->getShaderResourceView();
		}

		result = direct3dContext->Map(
			_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		CopyMemory(mappedResource.pData, directionalLight->getConstantBufferDataPointer(), sizeof(DirectionalLight::ConstantBufferData));
		direct3dContext->Unmap(_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER), 0);
	}


	// ポイントライト
	std::array<ID3D11ShaderResourceView*, PointLight::MAX_NUM> pointLightShadowCubeMapResourceView;
	for (size_t i = 0; i < PointLight::MAX_NUM; i++)
	{
		pointLightShadowCubeMapResourceView[i] = nullptr;

		const PointLight* pointLight = scene.getPointLight(i);
		if (pointLight != nullptr && pointLight->hasShadowMap())
		{
			pointLightShadowCubeMapResourceView[i] = pointLight->getShadowMapData().depthTexture->getShaderResourceView();
		}
	}

	result = direct3dContext->Map(
		_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);

	PointLight::ConstantBufferData* pointLightConstBufData = static_cast<PointLight::ConstantBufferData*>(mappedResource.pData);
	ZeroMemory(pointLightConstBufData, sizeof(PointLight::ConstantBufferData) * PointLight::MAX_NUM);

	size_t numPointLight = scene.getNumPointLight();
	for (size_t i = 0; i < numPointLight; i++)
	{
		const PointLight* pointLight = scene.getPointLight(i);
		if (pointLight != nullptr)
		{
			CopyMemory(&pointLightConstBufData[i], pointLight->getConstantBufferDataPointer(), sizeof(PointLight::ConstantBufferData));
		}
	}

	direct3dContext->Unmap(_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER), 0);


	// スポットライトの位置＆レンジの逆数のマップ
	std::array<ID3D11ShaderResourceView*, SpotLight::MAX_NUM> spotLightShadowMapResourceView;
	for (size_t i = 0; i < SpotLight::MAX_NUM; i++)
	{
		spotLightShadowMapResourceView[i] = nullptr;

		const SpotLight* spotLight = scene.getSpotLight(i);
		if (spotLight != nullptr && spotLight->hasShadowMap())
		{
			spotLightShadowMapResourceView[i] = spotLight->getShadowMapData().depthTexture->getShaderResourceView();
		}
	}

	result = direct3dContext->Map(
		_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);

	SpotLight::ConstantBufferData* spotLightConstBufData = static_cast<SpotLight::ConstantBufferData*>(mappedResource.pData);
	ZeroMemory(spotLightConstBufData, sizeof(SpotLight::ConstantBufferData) * SpotLight::MAX_NUM);

	size_t numSpotLight = scene.getNumSpotLight();
	for (size_t i = 0; i < numSpotLight; i++)
	{
		const SpotLight* spotLight = scene.getSpotLight(i);
		if (spotLight != nullptr)
		{
			CopyMemory(&spotLightConstBufData[i], spotLight->getConstantBufferDataPointer(), sizeof(SpotLight::ConstantBufferData));
		}
	}

	direct3dContext->Unmap(_d3dProgramForDeferredRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER), 0);

	// メッシュはひとつだけ
	UINT strides[1] = {sizeof(_quadrangle.topLeft)};
	UINT offsets[1] = {0};
	direct3dContext->IASetVertexBuffers(0, _d3dProgramForDeferredRendering.getVertexBuffers(0).size(), _d3dProgramForDeferredRendering.getVertexBuffers(0).data(), strides, offsets);
	direct3dContext->IASetInputLayout(_d3dProgramForDeferredRendering.getInputLayout());
	//direct3dContext->IASetInputLayout(nullptr);
	direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	_d3dProgramForDeferredRendering.setShadersToDirect3DContext(direct3dContext);
	_d3dProgramForDeferredRendering.setConstantBuffersToDirect3DContext(direct3dContext);

	ID3D11ShaderResourceView* gBufferShaderResourceViews[4] = {
		getGBufferDepthStencil()->getShaderResourceView(),
		getGBufferColorSpecularIntensity()->getShaderResourceView(),
		getGBufferNormal()->getShaderResourceView(),
		getGBufferSpecularPower()->getShaderResourceView(),
	};
	direct3dContext->PSSetShaderResources(0, 4, gBufferShaderResourceViews);

	ID3D11ShaderResourceView* shaderResourceViews[1] = {
		dirLightShadowMapResourceView,
	};
	direct3dContext->PSSetShaderResources(4, 1, shaderResourceViews);

	direct3dContext->PSSetShaderResources(5, pointLightShadowCubeMapResourceView.size(), pointLightShadowCubeMapResourceView.data());

	direct3dContext->PSSetShaderResources(5 + pointLightShadowCubeMapResourceView.size(), spotLightShadowMapResourceView.size(), spotLightShadowMapResourceView.data());

	// TODO:サンプラはテクスチャごとに作る必要はない
	ID3D11SamplerState* samplerStates[2] = {_pointSampler, _pcfSampler};
	direct3dContext->PSSetSamplers(0, 2, samplerStates);

	direct3dContext->Draw(4, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
	glUseProgram(_glProgramForDeferredRendering.getShaderProgram());
	GLProgram::checkGLError();

	glUniform1i(_glProgramForDeferredRendering.getUniformLocation(GLProgram::UNIFORM_NAME_RENDER_MODE), (GLint)_renderMode);

	Mat4 viewMatrix = Director::getCamera().getViewMatrix();
	viewMatrix.inverse();
	glUniformMatrix4fv(_glProgramForDeferredRendering.getUniformLocation("u_viewInverse"), 1, GL_FALSE, (GLfloat*)viewMatrix.m);
	glUniformMatrix4fv(_glProgramForDeferredRendering.getUniformLocation("u_depthTextureProjection"), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	GLProgram::checkGLError();

	static const Mat4& depthBiasMatrix = Mat4::createScale(Vec3(0.5f, 0.5f, 0.5f)) * Mat4::createTranslation(Vec3(1.0f, 1.0f, 1.0f));
	glUniformMatrix4fv(
		_glProgramForDeferredRendering.getUniformLocation("u_depthBiasMatrix"),
		1,
		GL_FALSE,
		(GLfloat*)depthBiasMatrix.m
	);
	GLProgram::checkGLError();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, getGBufferDepthStencil()->getTextureId());
	glUniform1i(_glProgramForDeferredRendering.getUniformLocation("u_gBufferDepthStencil"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, getGBufferColorSpecularIntensity()->getTextureId());
	glUniform1i(_glProgramForDeferredRendering.getUniformLocation("u_gBufferColorSpecularIntensity"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, getGBufferNormal()->getTextureId());
	glUniform1i(_glProgramForDeferredRendering.getUniformLocation("u_gBufferNormal"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, getGBufferSpecularPower()->getTextureId());
	glUniform1i(_glProgramForDeferredRendering.getUniformLocation("u_gBufferSpecularPower"), 3);

	glActiveTexture(GL_TEXTURE0);

	const Scene& scene = Director::getInstance()->getScene();

	// アンビエントライト
	const AmbientLight* ambientLight = scene.getAmbientLight();
	Logger::logAssert(ambientLight != nullptr, "シーンにアンビエントライトがない。");
	Color3B lightColor = ambientLight->getColor();
	float intensity = ambientLight->getIntensity();
	glUniform3f(_glProgramForDeferredRendering.getUniformLocation("u_ambientLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
	GLProgram::checkGLError();

	// ディレクショナルライト
	const DirectionalLight* directionalLight = scene.getDirectionalLight();
	if (directionalLight != nullptr)
	{
		glUniform1i(
			_glProgramForDeferredRendering.getUniformLocation("u_directionalLightIsValid"),
			1
		);
		GLProgram::checkGLError();

		lightColor = directionalLight->getColor();
		intensity = directionalLight->getIntensity();
		glUniform3f(_glProgramForDeferredRendering.getUniformLocation("u_directionalLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
		GLProgram::checkGLError();

		Vec3 direction = directionalLight->getDirection();
		direction.normalize();
		glUniform3fv(_glProgramForDeferredRendering.getUniformLocation("u_directionalLightDirection"), 1, (GLfloat*)&direction);
		GLProgram::checkGLError();

		glUniform1i(
			_glProgramForDeferredRendering.getUniformLocation("u_directionalLightHasShadowMap"),
			directionalLight->hasShadowMap()
		);

		if (directionalLight->hasShadowMap())
		{
			glUniformMatrix4fv(
				_glProgramForDeferredRendering.getUniformLocation("u_directionalLightViewMatrix"),
				1,
				GL_FALSE,
				(GLfloat*)directionalLight->getShadowMapData().viewMatrix.m
			);

			glUniformMatrix4fv(
				_glProgramForDeferredRendering.getUniformLocation("u_directionalLightProjectionMatrix"),
				1,
				GL_FALSE,
				(GLfloat*)directionalLight->getShadowMapData().projectionMatrix.m
			);

			glActiveTexture(GL_TEXTURE4);
			GLuint textureId = directionalLight->getShadowMapData().getDepthTexture()->getTextureId();
			glBindTexture(GL_TEXTURE_2D, textureId);
			glUniform1i(_glProgramForDeferredRendering.getUniformLocation("u_directionalLightShadowMap"), 4);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	// ポイントライト
	for (size_t i = 0; i < PointLight::MAX_NUM; i++)
	{
		//TODO:ユニフォームの変数名に[i]をつけねば
		const PointLight* pointLight = scene.getPointLight(i);
		if (pointLight != nullptr)
		{
			glUniform1i(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_pointLightIsValid[") + std::to_string(i) + std::string("]")).c_str()), 1);
			//glUniform1f(_glProgramForDeferredRendering.getUniformLocation(std::string("u_pointLightRangeInverse[") + std::to_string(i) + std::string("]")), 1.0f / pointLight->getRange());
			GLProgram::checkGLError();

			lightColor = pointLight->getColor();
			intensity = pointLight->getIntensity();
			glUniform3f(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_pointLightColor[") + std::to_string(i) + std::string("]")).c_str()), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			//glUniform3f(_glProgramForDeferredRendering.getUniformLocation(std::string("u_pointLightColor[") + std::to_string(i) + std::string("]")), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);

			GLProgram::checkGLError();

			glUniform3fv(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_pointLightPosition[") + std::to_string(i) + std::string("]")).c_str()), 1, (GLfloat*)&pointLight->getPosition()); // ライトについてはローカル座標でなくワールド座標である前提
			//glUniform3fv(_glProgramForDeferredRendering.getUniformLocation(std::string("u_pointLightPosition[") + std::to_string(i) + std::string("]")), 1, (GLfloat*)&pointLight->getPosition()); // ライトについてはローカル座標でなくワールド座標である前提
			GLProgram::checkGLError();

			glUniform1f(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_pointLightRangeInverse[") + std::to_string(i) + std::string("]")).c_str()), 1.0f / pointLight->getRange());
			//glUniform1f(_glProgramForDeferredRendering.getUniformLocation(std::string("u_pointLightRangeInverse[") + std::to_string(i) + std::string("]")), 1.0f / pointLight->getRange());
			GLProgram::checkGLError();

			glUniform1i(
				glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_pointLightHasShadowMap[") + std::to_string(i) + std::string("]")).c_str()),
				pointLight->hasShadowMap()
			);
			//glUniform1i(
			//	_glProgramForDeferredRendering.getUniformLocation(std::string("u_pointLightHasShadowMap[") + std::to_string(i) + std::string("]")),
			//	pointLight->hasShadowMap()
			//);

			if (pointLight->hasShadowMap())
			{
				glUniformMatrix4fv(
					glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_pointLightProjectionMatrix[") + std::to_string(i) + std::string("]")).c_str()),
					1,
					GL_FALSE,
					(GLfloat*)pointLight->getShadowMapData().projectionMatrix.m
				);
				//glUniformMatrix4fv(
				//	_glProgramForDeferredRendering.getUniformLocation(std::string("u_pointLightProjectionMatrix[") + std::to_string(i) + std::string("]")),
				//	1,
				//	GL_FALSE,
				//	(GLfloat*)pointLight->getShadowMapData().projectionMatrix.m
				//);

				for (int j = 0; j < (int)CubeMapFace::NUM_CUBEMAP_FACE; j++)
				{
					glUniformMatrix4fv(
						glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_pointLightViewMatrices[") + std::to_string(j) + std::string("][") + std::to_string(i) + std::string("]")).c_str()),
						1,
						GL_FALSE,
						(GLfloat*)pointLight->getShadowMapData().viewMatrices[j].m
					);
				}

				glActiveTexture(GL_TEXTURE5 + i);
				GLuint textureId = pointLight->getShadowMapData().getDepthTexture()->getTextureId();
				glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
				glUniform1i(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_pointLightShadowCubeMap[") + std::to_string(i) + std::string("]")).c_str()), 5 + i);
				//glUniform1i(_glProgramForDeferredRendering.getUniformLocation(std::string("u_pointLightShadowCubeMap[") + std::to_string(i) + std::string("]")), 5 + i);
				glActiveTexture(GL_TEXTURE0);
			}
		}
	}

	// スポットライト
	for (size_t i = 0; i < SpotLight::MAX_NUM; i++)
	{
		const SpotLight* spotLight = scene.getSpotLight(i);
		if (spotLight != nullptr)
		{
			glUniform1i(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightIsValid[") + std::to_string(i) + std::string("]")).c_str()), 1);
			GLProgram::checkGLError();

			lightColor = spotLight->getColor();
			intensity = spotLight->getIntensity();
			glUniform3f(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightColor[") + std::to_string(i) + std::string("]")).c_str()), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			//glUniform3f(_glProgramForDeferredRendering.getUniformLocation(std::string("u_spotLightColor[") + std::to_string(i) + std::string("]")), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			GLProgram::checkGLError();

			glUniform3fv(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightPosition[") + std::to_string(i) + std::string("]")).c_str()), 1, (GLfloat*)&spotLight->getPosition());
			//glUniform3fv(_glProgramForDeferredRendering.getUniformLocation(std::string("u_spotLightPosition[") + std::to_string(i) + std::string("]")), 1, (GLfloat*)&spotLight->getPosition());
			GLProgram::checkGLError();

			Vec3 direction = spotLight->getDirection();
			direction.normalize();
			glUniform3fv(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightDirection[") + std::to_string(i) + std::string("]")).c_str()), 1, (GLfloat*)&direction);
			//glUniform3fv(_glProgramForDeferredRendering.getUniformLocation(std::string("u_spotLightDirection[") + std::to_string(i) + std::string("]")), 1, (GLfloat*)&direction);
			GLProgram::checkGLError();

			glUniform1f(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightRangeInverse[") + std::to_string(i) + std::string("]")).c_str()), 1.0f / spotLight->getRange());
			//glUniform1f(_glProgramForDeferredRendering.getUniformLocation(std::string("u_spotLightRangeInverse[") + std::to_string(i) + std::string("]")), 1.0f / spotLight->getRange());
			GLProgram::checkGLError();

			glUniform1f(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightInnerAngleCos[") + std::to_string(i) + std::string("]")).c_str()), spotLight->getInnerAngleCos());
			//glUniform1f(_glProgramForDeferredRendering.getUniformLocation(std::string("u_spotLightInnerAngleCos[") + std::to_string(i) + std::string("]")), spotLight->getInnerAngleCos());
			GLProgram::checkGLError();

			glUniform1f(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightOuterAngleCos[") + std::to_string(i) + std::string("]")).c_str()), spotLight->getOuterAngleCos());
			//glUniform1f(_glProgramForDeferredRendering.getUniformLocation(std::string("u_spotLightOuterAngleCos[") + std::to_string(i) + std::string("]")), spotLight->getOuterAngleCos());
			GLProgram::checkGLError();

			glUniform1i(
				glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightHasShadowMap[") + std::to_string(i) + std::string("]")).c_str()),
				spotLight->hasShadowMap()
			);
			//glUniform1i(
			//	_glProgramForDeferredRendering.getUniformLocation(std::string("u_spotLightHasShadowMap[") + std::to_string(i) + std::string("]")),
			//	spotLight->hasShadowMap()
			//);

			if (spotLight->hasShadowMap())
			{
				glUniformMatrix4fv(
					glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightViewMatrix[") + std::to_string(i) + std::string("]")).c_str()),
					1,
					GL_FALSE,
					(GLfloat*)spotLight->getShadowMapData().viewMatrix.m
				);
				//glUniformMatrix4fv(
				//	_glProgramForDeferredRendering.getUniformLocation(std::string("u_spotLightViewMatrix[") + std::to_string(i) + std::string("]")),
				//	1,
				//	GL_FALSE,
				//	(GLfloat*)spotLight->getShadowMapData().viewMatrix.m
				//);

				glUniformMatrix4fv(
					glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightProjectionMatrix[") + std::to_string(i) + std::string("]")).c_str()),
					1,
					GL_FALSE,
					(GLfloat*)spotLight->getShadowMapData().projectionMatrix.m
				);
				//glUniformMatrix4fv(
				//	_glProgramForDeferredRendering.getUniformLocation(std::string("u_spotLightProjectionMatrix[") + std::to_string(i) + std::string("]")),
				//	1,
				//	GL_FALSE,
				//	(GLfloat*)spotLight->getShadowMapData().projectionMatrix.m
				//);

				glActiveTexture(GL_TEXTURE9 + i);
				GLuint textureId = spotLight->getShadowMapData().getDepthTexture()->getTextureId();
				glBindTexture(GL_TEXTURE_2D, textureId);
				glUniform1i(glGetUniformLocation(_glProgramForDeferredRendering.getShaderProgram(), (std::string("u_spotLightShadowMap[") + std::to_string(i) + std::string("]")).c_str()), 9 + i);
				//glUniform1i(_glProgramForDeferredRendering.getUniformLocation(std::string("u_spotLightShadowMap[") + std::to_string(i) + std::string("]")), 9 + i);
				glActiveTexture(GL_TEXTURE0);
			}
		}
	}

	glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
	GLProgram::checkGLError();

	glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
	GLProgram::checkGLError();

	glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.position);
	glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.textureCoordinate);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif
}
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

void Renderer::prepareFowardRendering()
{
	prepareDefaultRenderTarget();
}

void Renderer::prepareTransparentRendering()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	// デプスバッファがディファードレンダリング時にResourceViewに設定されているのでRenderTargetに設定するために解放
	ID3D11ShaderResourceView* resourceView[1] = { nullptr };
	_direct3dContext->PSSetShaderResources(0, 1, resourceView);

#if defined(MGRRENDERER_DEFERRED_RENDERING)
	// レンダーターゲットはカラーは通常描画と同じ。デプスはGバッファを参照する。
	_direct3dContext->OMSetRenderTargets(1, &_direct3dRenderTarget, _gBufferDepthStencil->getDepthStencilView());
#elif defined(MGRRENDERER_FORWARD_RENDERING)
	// レンダーターゲットは通常描画と同じ。
	_direct3dContext->OMSetRenderTargets(1, &_direct3dRenderTarget, _direct3dDepthStencilView);
#endif
	// デプステストはするがデプスは書き込まない
	_direct3dContext->OMSetDepthStencilState(_direct3dDepthStencilStateTransparent, 1);

	// ブレンドする
	FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	_direct3dContext->OMSetBlendState(_blendStateTransparent, blendFactor, 0xffffffff);
#elif defined(MGRRENDERER_USE_OPENGL)
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	// Gバッファのデプスバッファをデフォルトのデプスバッファにコピーする	
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _gBufferFrameBuffer->getFrameBufferId());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Write to default framebuffer
	const SizeUint& windowSize = Director::getInstance()->getWindowSize();
	glBlitFramebuffer(0, 0, windowSize.width, windowSize.height, 0, 0, windowSize.width, windowSize.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
}

void Renderer::prepareFowardRendering2D()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	// 2Dノードはワイアーフレーム描画からははずしておく
	_direct3dContext->RSSetState(_rasterizeStateNormal);

	// レンダーターゲットはカラーバッファは通常描画と同じだがデプステストをしないのでデプスバッファを外す。ブレンドは透過物パスと同様にブレンドを行う。
	_direct3dContext->OMSetRenderTargets(1, &_direct3dRenderTarget, nullptr);
	_direct3dContext->OMSetDepthStencilState(_direct3dDepthStencilState2D, 1);
#elif defined(MGRRENDERER_USE_OPENGL)
	// 2Dノードはワイアーフレーム描画からははずしておく
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
#endif
}

void Renderer::visitRenderQueue(const std::vector<RenderCommand*> queue)
{
	for (RenderCommand* command : queue)
	{
		// TODO:for-eachでなくもっとコレクションに対するパイプみたいなメソッド使いたいな
		executeRenderCommand(command);
	}
}

void Renderer::executeRenderCommand(RenderCommand* command)
{
	switch (command->getType())
	{
	case RenderCommand::Type::GROUP_BEGIN:
		{
			GroupBeginRenderCommand* groupCommand = static_cast<GroupBeginRenderCommand*>(command);
			groupCommand->execute();
			visitRenderQueue(_queueGroup[groupCommand->getGroupIndex()]);
		}
		break;
	case RenderCommand::Type::GROUP_END:
		{
			GroupBeginRenderCommand* groupCommand = static_cast<GroupBeginRenderCommand*>(command);
			groupCommand->execute();
		}
		break;
	case RenderCommand::Type::CUSTOM:
		command->execute();
		break;
	default:
		Logger::logAssert(false, "対応していないコマンドタイプが入力された。");
		break;
	}
}

} // namespace mgrrenderer
