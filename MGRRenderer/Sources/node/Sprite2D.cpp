#include "Sprite2D.h"
#include "utility/FileUtility.h"
#include "renderer/Image.h"
#include "renderer/Director.h"
#include "renderer/Shaders.h"
#include "renderer/TextureUtility.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "renderer/D3DTexture.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "renderer/GLTexture.h"
#endif

namespace mgrrenderer
{

const std::string Sprite2D::CONSTANT_BUFFER_DEPTH_TEXTURE_PARAMETER = "CONSTANT_BUFFER_DEPTH_TEXTURE_PARAMETER";
const std::string Sprite2D::CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX = "CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX";

Sprite2D::Sprite2D() :
_texture(nullptr),
_renderBufferType(RenderBufferType::NONE),
_isOwnTexture(false),
_nearClip(0.0f),
_farClip(0.0f),
_cubeMapFace(CubeMapFace::NONE)
{
}

Sprite2D::~Sprite2D()
{
	if (_isOwnTexture && _texture != nullptr)
	{
		delete _texture;
		_texture = nullptr;
	}
}

#if defined(MGRRENDERER_USE_DIRECT3D)
bool Sprite2D::initCommon(const std::string& path, const std::string& vertexShaderFunctionName, const std::string& geometryShaderFunctionName, const std::string& pixelShaderFunctionName, const SizeUint& contentSize)
{
	_quadrangle.bottomLeft.position = Vec2(0.0f, 0.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.bottomRight.position = Vec2((float)contentSize.width, 0.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0f, 1.0f);
	_quadrangle.topLeft.position = Vec2(0.0f, (float)contentSize.height);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.topRight.position = Vec2((float)contentSize.width, (float)contentSize.height);
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
	ID3D11Device* direct3dDevice = Director::getRenderer().getDirect3dDevice();
	ID3D11Buffer* vertexBuffer = nullptr;
	HRESULT result = direct3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubData, &vertexBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addVertexBuffer(vertexBuffer);

	// インデックスバッファ用の配列の用意。素直に昇順に番号付けする
	std::vector<unsigned int> indexArray;
	indexArray.resize(4);
	for (unsigned int i = 0; i < 4; i++)
	{
		indexArray[i] = i;
	}

	// インデックスバッファの定義
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(UINT) * 4;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// インデックスバッファのサブリソースの定義
	D3D11_SUBRESOURCE_DATA indexBufferSubData;
	indexBufferSubData.pSysMem = indexArray.data();
	indexBufferSubData.SysMemPitch = 0;
	indexBufferSubData.SysMemSlicePitch = 0;

	// インデックスバッファのサブリソースの作成
	ID3D11Buffer* indexBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferSubData, &indexBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.setIndexBuffer(indexBuffer);

	bool depthEnable = false;
	_d3dProgramForForwardRendering.initWithShaderFile(path, depthEnable, vertexShaderFunctionName, geometryShaderFunctionName, pixelShaderFunctionName);

	// 入力レイアウトオブジェクトの作成
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{D3DProgram::SEMANTIC_POSITION.c_str(), 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{D3DProgram::SEMANTIC_TEXTURE_COORDINATE.c_str(), 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(Vec2), D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	ID3D11InputLayout* inputLayout = nullptr;
	result = direct3dDevice->CreateInputLayout(
		layout,
		_countof(layout), 
		_d3dProgramForForwardRendering.getVertexShaderBlob()->GetBufferPointer(),
		_d3dProgramForForwardRendering.getVertexShaderBlob()->GetBufferSize(),
		&inputLayout
	);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateInputLayout failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.setInputLayout(inputLayout);

	// 定数バッファの作成
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	constantBufferDesc.ByteWidth = sizeof(Mat4);

	// Model行列用
	ID3D11Buffer* constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX, constantBuffer);

	// View行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);

	// Projection行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);
	return true;
}
#elif defined(MGRRENDERER_USE_OPENGL)
bool Sprite2D::initCommon(const std::string& geometryShaderFunctionPath, const std::string& pixelShaderFunctionPath, const SizeUint& contentSize)
{
	// 現状未使用
	(void)geometryShaderFunctionPath;

	_quadrangle.bottomLeft.position = Vec2(0.0f, 0.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.bottomRight.position = Vec2((float)contentSize.width, 0.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0f, 0.0f);
	_quadrangle.topLeft.position = Vec2(0.0f, (float)contentSize.height);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.topRight.position = Vec2((float)contentSize.width, (float)contentSize.height);
	_quadrangle.topRight.textureCoordinate = Vec2(1.0f, 1.0f);

	_glProgram.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderPositionTexture.glsl", pixelShaderFunctionPath);

	return true;
}
#endif

bool Sprite2D::init(const std::string& filePath)
{
	// Textureをロードし、pngやjpegを生データにし、OpenGLにあげる仕組みを作らねば。。Spriteのソースを見直すときだ。
	Image image; // ImageはCPU側のメモリを使っているのでこのスコープで解放されてもよいものだからスタックに取る
	image.initWithFilePath(filePath);

	// TextureはGPU側のメモリを使ってるので解放されると困るのでヒープにとる
#if defined(MGRRENDERER_USE_DIRECT3D)
	_texture = new D3DTexture();
#elif defined(MGRRENDERER_USE_OPENGL)
	_texture = new GLTexture();
#endif

	_isOwnTexture = true;

	Texture* texture = _texture;
	texture->initWithImage(image); // TODO:なぜか暗黙に継承元クラスのメソッドが呼べない

#if defined(MGRRENDERER_USE_DIRECT3D)
	bool success = initCommon("Resources/shader/PositionTextureMultiplyColor.hlsl", "VS", "", "PS", texture->getContentSize());
	if (!success)
	{
		return false;
	}

	// 乗算色
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	constantBufferDesc.ByteWidth = sizeof(Color4F); // getColor()のColor3Bにすると12バイト境界なので16バイト境界のためにパディングデータを作らねばならない
	ID3D11Buffer* constantBuffer = nullptr;
	HRESULT result = Director::getRenderer().getDirect3dDevice()->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);
	return true;
#elif defined(MGRRENDERER_USE_OPENGL)
	return initCommon("", "../MGRRenderer/Resources/shader/FragmentShaderPositionTextureMultiplyColor.glsl", texture->getContentSize());
#endif
}

#if defined(MGRRENDERER_USE_DIRECT3D)
bool Sprite2D::initWithTexture(D3DTexture* texture)
{
	_texture = texture;
	bool success = initCommon("Resources/shader/PositionTextureMultiplyColor.hlsl", "VS", "", "PS", texture->getContentSize());
	if (!success)
	{
		return false;
	}

	// 乗算色
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	constantBufferDesc.ByteWidth = sizeof(Color4F); // getColor()のColor3Bにすると12バイト境界なので16バイト境界のためにパディングデータを作らねばならない
	ID3D11Buffer* constantBuffer = nullptr;
	HRESULT result = Director::getRenderer().getDirect3dDevice()->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);

	return true;
}

bool Sprite2D::initWithRenderBuffer(D3DTexture* texture, RenderBufferType renderBufferType)
{
	_texture = texture;
	_renderBufferType = renderBufferType;

	// TODO:本当はこういうのでなくマテリアルをノードから切り離してマテリアルを引数で与えるようにしたい
	struct RenderBufferMaterial {
		std::string hlslFileName;
		std::string pixelShaderFunctionName;
	};

	static std::vector<RenderBufferMaterial> RenderBufferMaterialList = {
		{"Resources/shader/PositionRenderBufferMultiplyColor.hlsl", "PS_DEPTH_TEXTURE"},
		{"Resources/shader/PositionRenderBufferMultiplyColor.hlsl", "PS_DEPTH_TEXTURE_ORTHOGONAL"},
		{"Resources/shader/PositionRenderBufferMultiplyColor.hlsl", "PS_DEPTH_CUBEMAP_TEXTURE"},
		{"Resources/shader/PositionRenderBufferMultiplyColor.hlsl", "PS_GBUFFER_COLOR_SPECULAR_INTENSITY"},
		{"Resources/shader/PositionRenderBufferMultiplyColor.hlsl", "PS_GBUFFER_NORMAL"},
		{"Resources/shader/PositionRenderBufferMultiplyColor.hlsl", "PS_GBUFFER_SPECULAR_POWER"},
	};

	const RenderBufferMaterial& material = RenderBufferMaterialList[static_cast<int>(renderBufferType)];

	return initCommon(material.hlslFileName, "VS", "", material.pixelShaderFunctionName, texture->getContentSize());
}

bool Sprite2D::initWithDepthStencilTexture(D3DTexture* texture, RenderBufferType renderBufferType, float nearClip, float farClip, const Mat4& projectionMatrix, CubeMapFace face)
{
	Logger::logAssert(renderBufferType == RenderBufferType::DEPTH_TEXTURE || renderBufferType == RenderBufferType::DEPTH_TEXTURE_ORTHOGONAL || renderBufferType == RenderBufferType::DEPTH_CUBEMAP_TEXTURE, "レンダーバッファがデプスステンシルテクスチャでないのに専用の初期化メソッドを呼んだ。");

	_nearClip = nearClip;
	_farClip = farClip;
	_projectionMatrix = projectionMatrix;
	_cubeMapFace = face;

	bool success = initWithRenderBuffer(texture, renderBufferType);
	if (!success)
	{
		return false;
	}

	// デプステクスチャ描画時のProjection行列用
	ID3D11Buffer* constantBuffer = nullptr;

	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	constantBufferDesc.ByteWidth = sizeof(Vec4); // float一個しか必要ないが、16バイトアラインメントなので
	HRESULT result = Director::getRenderer().getDirect3dDevice()->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PARAMETER, constantBuffer);

	constantBufferDesc.ByteWidth = sizeof(Mat4);
	result = Director::getRenderer().getDirect3dDevice()->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX, constantBuffer);

	return true;
}
#elif defined(MGRRENDERER_USE_OPENGL)
bool Sprite2D::initWithRenderBuffer(GLTexture* texture, RenderBufferType renderBufferType)
{
	_texture = texture;
	_renderBufferType = renderBufferType;

	static std::vector<std::string> RenderBufferFSList = {
		"../MGRRenderer/Resources/shader/FragmentShaderDepthTexture.glsl",
		"../MGRRenderer/Resources/shader/FragmentShaderDepthTextureOrthogonal.glsl",
		"../MGRRenderer/Resources/shader/FragmentShaderDepthCubemapTexture.glsl",
		"../MGRRenderer/Resources/shader/FragmentShaderGBufferColorSpecularIntensity.glsl",
		"../MGRRenderer/Resources/shader/FragmentShaderGBufferNormal.glsl",
		"../MGRRenderer/Resources/shader/FragmentShaderGBufferSpecularPower.glsl",
	};

	const std::string& fragmentShader = RenderBufferFSList[static_cast<int>(renderBufferType)];

	return initCommon("", fragmentShader, texture->getContentSize());
}

bool Sprite2D::initWithDepthStencilTexture(GLTexture* texture, RenderBufferType renderBufferType, float nearClip, float farClip, const Mat4& projectionMatrix, CubeMapFace face)
{
	Logger::logAssert(renderBufferType == RenderBufferType::DEPTH_TEXTURE || renderBufferType == RenderBufferType::DEPTH_TEXTURE_ORTHOGONAL || renderBufferType == RenderBufferType::DEPTH_CUBEMAP_TEXTURE, "レンダーバッファがデプスステンシルテクスチャでないのに専用の初期化メソッドを呼んだ。");

	_nearClip = nearClip;
	_farClip = farClip;
	_projectionMatrix = projectionMatrix;
	_cubeMapFace = face;

	return initWithRenderBuffer(texture, renderBufferType);
}
#endif

#if defined(MGRRENDERER_DEFERRED_RENDERING)
void Sprite2D::renderGBuffer()
{
	Node::renderGBuffer();
}
#endif

void Sprite2D::renderForward(bool isTransparent)
{
	(void)isTransparent;

	_renderForwardCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();

		// TODO:ここらへん共通化したいな。。
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// モデル行列のマップ
		HRESULT result = direct3dContext->Map(
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 modelMatrix = getModelMatrix().createTranspose();
		CopyMemory(mappedResource.pData, &modelMatrix.m, sizeof(modelMatrix));
		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX), 0);

		// ビュー行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 viewMatrix = Director::getCameraFor2D().getViewMatrix().createTranspose();
		CopyMemory(mappedResource.pData, &viewMatrix.m, sizeof(viewMatrix));
		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX), 0);

		// プロジェクション行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 projectionMatrix = (Mat4::CHIRARITY_CONVERTER * Director::getCameraFor2D().getProjectionMatrix()).transpose();
		CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

		// デプステクスチャ描画時のプロジェクション行列の情報のマップ
		switch (_renderBufferType) {
			case RenderBufferType::DEPTH_TEXTURE:
			case RenderBufferType::DEPTH_TEXTURE_ORTHOGONAL:
			case RenderBufferType::DEPTH_CUBEMAP_TEXTURE:
			{
				result = direct3dContext->Map(
					_d3dProgramForForwardRendering.getConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PARAMETER),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				// nearClip, farClipの値を正にしているときは右手系ではzは負。zの値を渡す

				struct Parameter {
					float nearClip;
					float farClip;
					unsigned int faceIndex;
					float padding;
				} parameter = {-_nearClip, -_farClip, (unsigned int)_cubeMapFace, 0.0f};
				CopyMemory(mappedResource.pData, &parameter, sizeof(parameter));
				direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PARAMETER), 0);

				result = direct3dContext->Map(
					_d3dProgramForForwardRendering.getConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				Mat4 depthProjectionMatrix = (Mat4::CHIRARITY_CONVERTER * _projectionMatrix).transpose();
				CopyMemory(mappedResource.pData, &depthProjectionMatrix.m, sizeof(depthProjectionMatrix));
				direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX), 0);
			}
				break;
			case RenderBufferType::GBUFFER_COLOR_SPECULAR_INTENSITY:
			case RenderBufferType::GBUFFER_NORMAL:
			case RenderBufferType::GBUFFER_SPECULAR_POWER:
				break;
			default:
			{
				// 乗算色のマップ
				result = direct3dContext->Map(
					_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				const Color4F& multiplyColor = Color4F(Color4B(getColor().r, getColor().g, getColor().b, 255));
				CopyMemory(mappedResource.pData, &multiplyColor , sizeof(multiplyColor));
				direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR), 0);
				break;
			}
		}
		
		UINT strides[1] = {sizeof(_quadrangle.topLeft)};
		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForForwardRendering.getVertexBuffers().size(), _d3dProgramForForwardRendering.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForForwardRendering.getIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForForwardRendering.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		_d3dProgramForForwardRendering.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForForwardRendering.setConstantBuffersToDirect3DContext(direct3dContext);

		UINT startSlot = 0;
		// キューブマップテクスチャは別のスロットを使う
		if (_renderBufferType == RenderBufferType::DEPTH_CUBEMAP_TEXTURE)
		{
			startSlot = 1;
		}

		ID3D11ShaderResourceView* resourceView[1] = { _texture->getShaderResourceView() };
		direct3dContext->PSSetShaderResources(startSlot, 1, resourceView);
		ID3D11SamplerState* samplerState[1] = { Director::getRenderer().getLinearSamplerState() };
		direct3dContext->PSSetSamplers(0, 1, samplerState);

		direct3dContext->DrawIndexed(4, 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		// cocos2d-xはTriangleCommand発行してる形だからな。。テクスチャバインドはTexture2Dでやってるのに大丈夫か？
		glUseProgram(_glProgram.getShaderProgram());
		GLProgram::checkGLError();

		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getProjectionMatrix().m);
		GLProgram::checkGLError();

		// デプステクスチャ描画時のプロジェクション行列のマップ
		switch (_renderBufferType) {
			case RenderBufferType::DEPTH_CUBEMAP_TEXTURE:
				glUniform1i(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_CUBEMAP_FACE), (GLint)_cubeMapFace);
				GLProgram::checkGLError();
				// そのまま通過する
			case RenderBufferType::DEPTH_TEXTURE:
			case RenderBufferType::DEPTH_TEXTURE_ORTHOGONAL:
			{
				glUniform1f(_glProgram.getUniformLocation("u_nearClipZ"), -_nearClip);
				GLProgram::checkGLError();

				glUniform1f(_glProgram.getUniformLocation("u_farClipZ"), -_farClip);
				GLProgram::checkGLError();

				glUniformMatrix4fv(_glProgram.getUniformLocation("u_depthTextureProjectionMatrix"), 1, GL_FALSE, (GLfloat*)_projectionMatrix.m);
				GLProgram::checkGLError();
				break;
			}
			case RenderBufferType::GBUFFER_COLOR_SPECULAR_INTENSITY:
			case RenderBufferType::GBUFFER_NORMAL:
			case RenderBufferType::GBUFFER_SPECULAR_POWER:
				break;
			default:
				glUniform3f(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
				GLProgram::checkGLError();
				break;
		}

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
		GLProgram::checkGLError();

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.position);
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.textureCoordinate);

		if (_renderBufferType == RenderBufferType::DEPTH_CUBEMAP_TEXTURE)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, _texture->getTextureId());
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		}
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif
	});

	Director::getRenderer().addCommand(&_renderForwardCommand);
}

} // namespace mgrrenderer
