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
#if defined(MGRRENDERER_USE_DIRECT3D)
_pointSampler(nullptr),
_linearSampler(nullptr),
_pcfSampler(nullptr),
_rasterizeStateNormal(nullptr),
_rasterizeStateCullFaceFront(nullptr),
_rasterizeStateCullFaceBack(nullptr),
_gBufferDepthStencil(nullptr),
_gBufferColorSpecularIntensity(nullptr),
_gBufferNormal(nullptr),
_gBufferSpecularPower(nullptr)
#elif defined(MGRRENDERER_USE_OPENGL)
_gBufferFrameBuffer(nullptr)
#endif
{
	_groupIndexStack.push(DEFAULT_RENDER_QUEUE_GROUP_INDEX);

	std::vector<RenderCommand*> defaultRenderQueue;
	_queueGroup.push_back(defaultRenderQueue);
}

Renderer::~Renderer()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
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

	if (_gBufferDepthStencil != nullptr)
	{
		delete _gBufferDepthStencil;
		_gBufferDepthStencil = nullptr;
	}

	if (_rasterizeStateCullFaceBack != nullptr)
	{
		_rasterizeStateCullFaceBack->Release();
		_rasterizeStateCullFaceBack = nullptr;
	}

	if (_rasterizeStateCullFaceFront != nullptr)
	{
		_rasterizeStateCullFaceFront ->Release();
		_rasterizeStateCullFaceFront  = nullptr;
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
#elif defined(MGRRENDERER_USE_OPENGL)
	if (_gBufferFrameBuffer != nullptr)
	{
		delete _gBufferFrameBuffer;
		_gBufferFrameBuffer = nullptr;
	}
#endif
}

void Renderer::initView(const Size& windowSize)
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	// ビューポートの準備
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
	direct3dContext->RSSetViewports(1, Director::getInstance()->getDirect3dViewport());

	// 汎用サンプラの用意
	D3D11_SAMPLER_DESC desc;
	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = 1;
	desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	desc.BorderColor[0] = 0.0f;
	desc.BorderColor[1] = 0.0f;
	desc.BorderColor[2] = 0.0f;
	desc.BorderColor[3] = 0.0f;
	desc.MinLOD = -FLT_MAX;
	desc.MaxLOD = FLT_MAX;
	HRESULT result = Director::getInstance()->getDirect3dDevice()->CreateSamplerState(&desc, &_pointSampler);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateSamplerState failed. result=%d", result);
		return;
	}

	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	result = Director::getInstance()->getDirect3dDevice()->CreateSamplerState(&desc, &_linearSampler);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateSamplerState failed. result=%d", result);
		return;
	}

	desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	result = Director::getInstance()->getDirect3dDevice()->CreateSamplerState(&desc, &_pcfSampler);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateSamplerState failed. result=%d", result);
		return;
	}

	// ラスタライズステートオブジェクトの作成　フェイスカリングなし
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
	result = Director::getInstance()->getDirect3dDevice()->CreateRasterizerState(&rasterizeDesc, &_rasterizeStateNormal);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateRasterizerState failed. result=%d", result);
		return;
	}

	rasterizeDesc.CullMode = D3D11_CULL_FRONT;
	result = Director::getInstance()->getDirect3dDevice()->CreateRasterizerState(&rasterizeDesc, &_rasterizeStateCullFaceFront);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateRasterizerState failed. result=%d", result);
		return;
	}

	rasterizeDesc.CullMode = D3D11_CULL_BACK;
	result = Director::getInstance()->getDirect3dDevice()->CreateRasterizerState(&rasterizeDesc, &_rasterizeStateCullFaceBack);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateRasterizerState failed. result=%d", result);
		return;
	}

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

	_d3dProgram.initWithShaderFile("Resources/shader/DeferredLighting.hlsl", true, "VS", "", "PS");

	// 定数バッファの作成
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	constantBufferDesc.ByteWidth = sizeof(Mat4);

	ID3D11Buffer* constantBuffer = nullptr;
	ID3D11Device* direct3dDevice = Director::getInstance()->getDirect3dDevice();
	// View行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);

	// Projection行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);

	// アンビエントライトカラー
	constantBufferDesc.ByteWidth = sizeof(AmbientLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer);

	// ディレクショナルトライトView行列用
	constantBufferDesc.ByteWidth = sizeof(Mat4);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX, constantBuffer);

	// ディレクショナルトライトProjection行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX, constantBuffer);

	// ディレクショナルトライトデプスバイアス行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_DEPTH_BIAS_MATRIX, constantBuffer);

	// ディレクショナルトライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(DirectionalLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer);

	// ポイントライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(PointLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer);

	// スポットライトView行列用
	constantBufferDesc.ByteWidth = sizeof(Mat4);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_VIEW_MATRIX, constantBuffer);

	// スポットライトProjection行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PROJECTION_MATRIX, constantBuffer);

	// スポットライトデプスバイアス行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_DEPTH_BIAS_MATRIX, constantBuffer);

	// スポットライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(SpotLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER, constantBuffer);

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
	result = direct3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubData, &vertexBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addVertexBuffer(vertexBuffer);

	// 入力レイアウトオブジェクトの作成
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{D3DProgram::SEMANTIC_POSITION.c_str(), 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{D3DProgram::SEMANTIC_TEXTURE_COORDINATE.c_str(), 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(Vec2), D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	ID3D11InputLayout* inputLayout = nullptr;
	result = direct3dDevice->CreateInputLayout(
		layout,
		_countof(layout), 
		_d3dProgram.getVertexShaderBlob()->GetBufferPointer(),
		_d3dProgram.getVertexShaderBlob()->GetBufferSize(),
		&inputLayout
	);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateInputLayout failed. result=%d", result);
		return;
	}
	_d3dProgram.setInputLayout(inputLayout);
#elif defined(MGRRENDERER_USE_OPENGL)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// TODO:ブレンドが必要ない時もブレンドをONにしている
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// OpenGL側でやるビューポート変換のためのパラメータを渡す
	glViewport(0, 0, static_cast<GLsizei>(windowSize.width), static_cast<GLsizei>(windowSize.height));
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // デフォルトのフレームバッファ

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
	_gBufferFrameBuffer->initWithTextureParams(drawBuffers, pixelFormats, false, windowSize);


	//
	// ディファードレンダリングの準備
	//
	//_d3dProgram.initWithShaderFile("Resources/shader/DeferredLighting.hlsl", true, "VS", "", "PS");

	_glProgram.initWithShaderString(shader::VERTEX_SHADER_DEFERRED_LIGHTING, shader::FRAGMENT_SHADER_DEFERRED_LIGHTING);

	_quadrangle.bottomLeft.position = Vec2(-1.0f, -1.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.bottomRight.position = Vec2(1.0, -1.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0, 0.0f);
	_quadrangle.topLeft.position = Vec2(-1.0f, 1.0f);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.topRight.position = Vec2(1.0, 1.0);
	_quadrangle.topRight.textureCoordinate = Vec2(1.0f, 1.0f);
#endif
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

void Renderer::prepareGBufferRendering()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
	direct3dContext->ClearState();

	float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	direct3dContext->ClearRenderTargetView(_gBufferColorSpecularIntensity->getRenderTargetView(), clearColor);
	direct3dContext->ClearRenderTargetView(_gBufferNormal->getRenderTargetView(), clearColor);
	direct3dContext->ClearRenderTargetView(_gBufferSpecularPower->getRenderTargetView(), clearColor);
	direct3dContext->ClearDepthStencilView(_gBufferDepthStencil->getDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	direct3dContext->RSSetViewports(1, Director::getInstance()->getDirect3dViewport());
	direct3dContext->RSSetState(getRasterizeStateCullFaceNormal());

	ID3D11RenderTargetView* gBuffers[3] = {_gBufferColorSpecularIntensity->getRenderTargetView(), _gBufferNormal->getRenderTargetView(), _gBufferSpecularPower->getRenderTargetView()};
	direct3dContext->OMSetRenderTargets(3, gBuffers, _gBufferDepthStencil->getDepthStencilView());
	direct3dContext->OMSetDepthStencilState(_gBufferDepthStencil->getDepthStencilState(), 1);
#elif defined(MGRRENDERER_USE_OPENGL)
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
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
	direct3dContext->ClearState();

	float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	direct3dContext->ClearRenderTargetView(Director::getInstance()->getDirect3dRenderTarget(), clearColor);
	direct3dContext->ClearDepthStencilView(Director::getInstance()->getDirect3dDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	direct3dContext->RSSetViewports(1, Director::getInstance()->getDirect3dViewport());
	ID3D11RenderTargetView* renderTarget = Director::getInstance()->getDirect3dRenderTarget(); //TODO: 一度変数に入れないとコンパイルエラーが出てしまった
	direct3dContext->RSSetState(Director::getRenderer().getRasterizeStateCullFaceNormal());

	direct3dContext->OMSetRenderTargets(1, &renderTarget, Director::getInstance()->getDirect3dDepthStencilView());
	direct3dContext->OMSetDepthStencilState(Director::getInstance()->getDirect3dDepthStencilState(), 1);
#elif defined(MGRRENDERER_USE_OPENGL)
	//glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glViewport(0, 0, static_cast<GLsizei>(Director::getInstance()->getWindowSize().width), static_cast<GLsizei>(Director::getInstance()->getWindowSize().height));
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // デフォルトフレームバッファに戻す
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void Renderer::renderDeferred()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();

	// TODO:ここらへん共通化したいな。。
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// ビュー行列の逆行列のマップ
	HRESULT result = direct3dContext->Map(
		_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
	Mat4 viewMatrix = Director::getCamera().getViewMatrix();
	viewMatrix.inverse();
	viewMatrix.transpose(); // Direct3Dでは転置した状態で入れる
	CopyMemory(mappedResource.pData, &viewMatrix.m, sizeof(viewMatrix));
	direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX), 0);

	// プロジェクション行列の逆行列のマップ
	result = direct3dContext->Map(
		_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
	Mat4 projectionMatrix = Director::getCamera().getProjectionMatrix();
	projectionMatrix.transpose();
	CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
	direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

	ID3D11ShaderResourceView* shadowMapResourceView = nullptr;
	// ライトの設定
	// TODO:現状、ライトは各種類ごとに一個ずつしか処理してない。最後のやつで上書き。
	for (Light* light : Director::getLight())
	{
		switch (light->getLightType())
		{
		case LightType::AMBIENT:
		{
			// アンビエントライトカラーのマップ
			result = direct3dContext->Map(
				_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
			CopyMemory(mappedResource.pData, light->getConstantBufferDataPointer(), sizeof(AmbientLight::ConstantBufferData));
			direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER), 0);
		}
			break;
		case LightType::DIRECTION:
		{
			if (light->hasShadowMap())
			{
				result = direct3dContext->Map(
					_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				Mat4 lightViewMatrix = light->getShadowMapData().viewMatrix;
				lightViewMatrix.transpose();
				CopyMemory(mappedResource.pData, &lightViewMatrix.m, sizeof(lightViewMatrix));
				direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX), 0);

				result = direct3dContext->Map(
					_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				Mat4 lightProjectionMatrix = light->getShadowMapData().projectionMatrix;
				lightProjectionMatrix = Mat4::CHIRARITY_CONVERTER * lightProjectionMatrix; // 左手系変換行列はプロジェクション行列に最初からかけておく
				lightProjectionMatrix.transpose();
				CopyMemory(mappedResource.pData, &lightProjectionMatrix.m, sizeof(lightProjectionMatrix));
				direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX), 0);

				result = direct3dContext->Map(
					_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_DEPTH_BIAS_MATRIX),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				Mat4 depthBiasMatrix = (Mat4::TEXTURE_COORDINATE_CONVERTER * Mat4::createScale(Vec3(0.5f, 0.5f, 1.0f)) * Mat4::createTranslation(Vec3(1.0f, -1.0f, 0.0f))).transpose(); //TODO: Mat4を参照型にすると値がおかしくなってしまう
				CopyMemory(mappedResource.pData, &depthBiasMatrix.m, sizeof(depthBiasMatrix));
				direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_DEPTH_BIAS_MATRIX), 0);

				shadowMapResourceView = light->getShadowMapData().depthTexture->getShaderResourceView();
			}

			result = direct3dContext->Map(
				_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
			CopyMemory(mappedResource.pData, light->getConstantBufferDataPointer(), sizeof(DirectionalLight::ConstantBufferData));
			direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER), 0);
		}
			break;
		case LightType::POINT: {
			if (light->hasShadowMap())
			{
				shadowMapResourceView = light->getShadowMapData().depthTexture->getShaderResourceView();
			}
			result = direct3dContext->Map(
				_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
			CopyMemory(mappedResource.pData, light->getConstantBufferDataPointer(), sizeof(PointLight::ConstantBufferData));
			direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER), 0);
		}
			break;
		case LightType::SPOT: {
			if (light->hasShadowMap())
			{
				result = direct3dContext->Map(
					_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_VIEW_MATRIX),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				Mat4 lightViewMatrix = light->getShadowMapData().viewMatrix;
				lightViewMatrix.transpose();
				CopyMemory(mappedResource.pData, &lightViewMatrix.m, sizeof(lightViewMatrix));
				direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_VIEW_MATRIX), 0);

				result = direct3dContext->Map(
					_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PROJECTION_MATRIX),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				Mat4 lightProjectionMatrix = light->getShadowMapData().projectionMatrix;
				lightProjectionMatrix = Mat4::CHIRARITY_CONVERTER * lightProjectionMatrix; // 左手系変換行列はプロジェクション行列に最初からかけておく
				lightProjectionMatrix.transpose();
				CopyMemory(mappedResource.pData, &lightProjectionMatrix.m, sizeof(lightProjectionMatrix));
				direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PROJECTION_MATRIX), 0);

				result = direct3dContext->Map(
					_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_DEPTH_BIAS_MATRIX),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				Mat4 depthBiasMatrix = (Mat4::TEXTURE_COORDINATE_CONVERTER * Mat4::createScale(Vec3(0.5f, 0.5f, 1.0f)) * Mat4::createTranslation(Vec3(1.0f, -1.0f, 0.0f))).transpose(); //TODO: Mat4を参照型にすると値がおかしくなってしまう
				CopyMemory(mappedResource.pData, &depthBiasMatrix.m, sizeof(depthBiasMatrix));
				direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_DEPTH_BIAS_MATRIX), 0);

				shadowMapResourceView = light->getShadowMapData().depthTexture->getShaderResourceView();
			}

			// スポットライトの位置＆レンジの逆数のマップ
			result = direct3dContext->Map(
				_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			CopyMemory(mappedResource.pData, light->getConstantBufferDataPointer(), sizeof(SpotLight::ConstantBufferData));
			direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER), 0);
		}
			break;
		}
	}

	UINT strides[1] = {sizeof(_quadrangle.topLeft)};
	UINT offsets[1] = {0};
	direct3dContext->IASetVertexBuffers(0, _d3dProgram.getVertexBuffers().size(), _d3dProgram.getVertexBuffers().data(), strides, offsets);
	direct3dContext->IASetInputLayout(_d3dProgram.getInputLayout());
	//direct3dContext->IASetInputLayout(nullptr);
	direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	_d3dProgram.setShadersToDirect3DContext(direct3dContext);
	_d3dProgram.setConstantBuffersToDirect3DContext(direct3dContext);

	if (shadowMapResourceView == nullptr)
	{
		ID3D11ShaderResourceView* gBufferShaderResourceViews[4] = {
			getGBufferDepthStencil()->getShaderResourceView(),
			getGBufferColorSpecularIntensity()->getShaderResourceView(),
			getGBufferNormal()->getShaderResourceView(),
			getGBufferSpecularPower()->getShaderResourceView(),
		};
		direct3dContext->PSSetShaderResources(0, 4, gBufferShaderResourceViews);
	}
	else
	{
		ID3D11ShaderResourceView* gBufferShaderResourceViews[5] = {
			getGBufferDepthStencil()->getShaderResourceView(),
			getGBufferColorSpecularIntensity()->getShaderResourceView(),
			getGBufferNormal()->getShaderResourceView(),
			getGBufferSpecularPower()->getShaderResourceView(),
			shadowMapResourceView,
		};
		direct3dContext->PSSetShaderResources(0, 5, gBufferShaderResourceViews);
	}

	// TODO:サンプラはテクスチャごとに作る必要はない
	ID3D11SamplerState* samplerStates[2] = {_pointSampler, _pcfSampler};
	direct3dContext->PSSetSamplers(0, 2, samplerStates);

	FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	direct3dContext->OMSetBlendState(_d3dProgram.getBlendState(), blendFactor, 0xffffffff);

	direct3dContext->Draw(4, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
	glUseProgram(_glProgram.getShaderProgram());
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	Mat4 viewMatrix = Director::getCamera().getViewMatrix();
	viewMatrix.inverse();
	glUniformMatrix4fv(_glProgram.getUniformLocation("u_viewInverse"), 1, GL_FALSE, (GLfloat*)viewMatrix.m);
	glUniformMatrix4fv(_glProgram.getUniformLocation("u_depthTextureProjection"), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, getGBuffers()[0]->getTextureId());
	glUniform1i(_glProgram.getUniformLocation("u_gBufferDepthStencil"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, getGBuffers()[1]->getTextureId());
	glUniform1i(_glProgram.getUniformLocation("u_gBufferColorSpecularIntensity"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, getGBuffers()[2]->getTextureId());
	glUniform1i(_glProgram.getUniformLocation("u_gBufferNormal"), 2);

	//glActiveTexture(GL_TEXTURE3);
	//glBindTexture(GL_TEXTURE_2D, getGBuffers()[3]->getTextureId());
	//glUniform1i(_glProgram.getUniformLocation("u_gBufferSpecularPower"), 3);

	glActiveTexture(GL_TEXTURE0);

	// ライトの設定
	// TODO:現状、ライトは各種類ごとに一個ずつしか処理してない。最後のやつで上書き。
	for (Light* light : Director::getLight())
	{
		const Color3B& lightColor = light->getColor();
		float intensity = light->getIntensity();

		switch (light->getLightType())
		{
		case LightType::AMBIENT:
			glUniform3f(_glProgram.getUniformLocation("u_ambientLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
			break;
		case LightType::DIRECTION: {
			glUniform3f(_glProgram.getUniformLocation("u_directionalLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
			Vec3 direction = dirLight->getDirection();
			direction.normalize();
			glUniform3fv(_glProgram.getUniformLocation("u_directionalLightDirection"), 1, (GLfloat*)&direction);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform1i(
				_glProgram.getUniformLocation("u_directionalLightHasShadowMap"),
				dirLight->hasShadowMap()
			);

			// TODO:とりあえず影つけはDirectionalLightのみを想定
			if (dirLight->hasShadowMap())
			{
				glUniformMatrix4fv(
					_glProgram.getUniformLocation("u_lightViewMatrix"),
					1,
					GL_FALSE,
					(GLfloat*)dirLight->getShadowMapData().viewMatrix.m
				);

				glUniformMatrix4fv(
					_glProgram.getUniformLocation("u_lightProjectionMatrix"),
					1,
					GL_FALSE,
					(GLfloat*)dirLight->getShadowMapData().projectionMatrix.m
				);

				static const Mat4& depthBiasMatrix = Mat4::createScale(Vec3(0.5f, 0.5f, 0.5f)) * Mat4::createTranslation(Vec3(1.0f, 1.0f, 1.0f));

				glUniformMatrix4fv(
					_glProgram.getUniformLocation("u_depthBiasMatrix"),
					1,
					GL_FALSE,
					(GLfloat*)depthBiasMatrix.m
				);

				glActiveTexture(GL_TEXTURE4);
				GLuint textureId = dirLight->getShadowMapData().getDepthTexture()->getTextureId();
				glBindTexture(GL_TEXTURE_2D, textureId);
				glUniform1i(_glProgram.getUniformLocation("u_shadowTexture"), 4);
				glActiveTexture(GL_TEXTURE0);
			}
		}
			break;
		case LightType::POINT: {
			glUniform3f(_glProgram.getUniformLocation("u_pointLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);

			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform3fv(_glProgram.getUniformLocation("u_pointLightPosition"), 1, (GLfloat*)&light->getPosition()); // ライトについてはローカル座標でなくワールド座標である前提
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			PointLight* pointLight = static_cast<PointLight*>(light);
			glUniform1f(_glProgram.getUniformLocation("u_pointLightRangeInverse"), 1.0f / pointLight->getRange());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		}
			break;
		case LightType::SPOT: {
			glUniform3f(_glProgram.getUniformLocation("u_spotLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform3fv(_glProgram.getUniformLocation("u_spotLightPosition"), 1, (GLfloat*)&light->getPosition());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			SpotLight* spotLight = static_cast<SpotLight*>(light);
			Vec3 direction = spotLight->getDirection();
			direction.normalize();
			glUniform3fv(_glProgram.getUniformLocation("u_spotLightDirection"), 1, (GLfloat*)&direction);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform1f(_glProgram.getUniformLocation("u_spotLightRangeInverse"), 1.0f / spotLight->getRange());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform1f(_glProgram.getUniformLocation("u_spotLightInnerAngleCos"), spotLight->getInnerAngleCos());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform1f(_glProgram.getUniformLocation("u_spotLightOuterAngleCos"), spotLight->getOuterAngleCos());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		}
			break;
		default:
			break;
		}
	}

	glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.position);
	glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.textureCoordinate);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif
}

void Renderer::prepareFowardRendering()
{
	// 現状特にすることがない。ここだけprepareがないと対称でないため定義した
}

void Renderer::prepareFowardRendering2D()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	Director::getInstance()->getDirect3dContext()->OMSetDepthStencilState(Director::getInstance()->getDirect3dDepthStencilState2D(), 1);
#elif defined(MGRRENDERER_USE_OPENGL)
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
