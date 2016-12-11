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
bool Sprite2D::initCommon(const std::string& path, const std::string& vertexShaderFunctionName, const std::string& geometryShaderFunctionName, const std::string& pixelShaderFunctionName, const Size& contentSize)
{
	_quadrangle.bottomLeft.position = Vec2(0.0f, 0.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.bottomRight.position = Vec2(contentSize.width, 0.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0f, 1.0f);
	_quadrangle.topLeft.position = Vec2(0.0f, contentSize.height);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.topRight.position = Vec2(contentSize.width, contentSize.height);
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
	ID3D11Device* direct3dDevice = Director::getInstance()->getDirect3dDevice();
	ID3D11Buffer* vertexBuffer = nullptr;
	HRESULT result = direct3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubData, &vertexBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addVertexBuffer(vertexBuffer);

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
	_d3dProgram.setIndexBuffer(indexBuffer);

	bool depthEnable = false;
	_d3dProgram.initWithShaderFile(path, depthEnable, vertexShaderFunctionName, geometryShaderFunctionName, pixelShaderFunctionName);

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
		return false;
	}
	_d3dProgram.setInputLayout(inputLayout);

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
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX, constantBuffer);

	// View行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);

	// Projection行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);
	return true;
}
#elif defined(MGRRENDERER_USE_OPENGL)
bool Sprite2D::initCommon(const char* geometryShaderFunctionName, const char* pixelShaderFunctionName, const Size& contentSize)
{
	// 現状未使用
	(void)geometryShaderFunctionName;

	_quadrangle.bottomLeft.position = Vec2(0.0f, 0.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.bottomRight.position = Vec2(contentSize.width, 0.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0f, 0.0f);
	_quadrangle.topLeft.position = Vec2(0.0f, contentSize.height);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.topRight.position = Vec2(contentSize.width, contentSize.height);
	_quadrangle.topRight.textureCoordinate = Vec2(1.0f, 1.0f);

	_glProgram.initWithShaderString(shader::VERTEX_SHADER_POSITION_TEXTURE, pixelShaderFunctionName);

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
	HRESULT result = Director::getInstance()->getDirect3dDevice()->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);
	return true;
#elif defined(MGRRENDERER_USE_OPENGL)
	return initCommon("", shader::FRAGMENT_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR, texture->getContentSize());
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
	HRESULT result = Director::getInstance()->getDirect3dDevice()->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);

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
	HRESULT result = Director::getInstance()->getDirect3dDevice()->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PARAMETER, constantBuffer);

	constantBufferDesc.ByteWidth = sizeof(Mat4);
	result = Director::getInstance()->getDirect3dDevice()->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX, constantBuffer);

	return true;
}
#elif defined(MGRRENDERER_USE_OPENGL)
bool Sprite2D::initWithRenderBuffer(GLTexture* texture, RenderBufferType renderBufferType)
{
	_texture = texture;
	_renderBufferType = renderBufferType;

	static std::vector<const char*> RenderBufferFSList = {
		shader::FRAGMENT_SHADER_DEPTH_TEXTURE,
		"", //TODO:まだ未対応
		shader::FRAGMENT_SHADER_GBUFFER_COLOR_SPECULAR_INTENSITY,
		shader::FRAGMENT_SHADER_GBUFFER_NORMAL,
		shader::FRAGMENT_SHADER_GBUFFER_SPECULAR_POWER,
	};

	const char* fragmentShader = RenderBufferFSList[static_cast<int>(renderBufferType)];

	return initCommon("", fragmentShader, texture->getContentSize());
}
#endif

void Sprite2D::renderGBuffer()
{
	Node::renderGBuffer();
}

void Sprite2D::renderForward()
{
	_renderCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();

		// TODO:ここらへん共通化したいな。。
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// モデル行列のマップ
		HRESULT result = direct3dContext->Map(
			_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 modelMatrix = getModelMatrix();
		modelMatrix.transpose();
		CopyMemory(mappedResource.pData, &modelMatrix.m, sizeof(modelMatrix));
		direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX), 0);

		// ビュー行列のマップ
		result = direct3dContext->Map(
			_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 viewMatrix = Director::getCameraFor2D().getViewMatrix();
		viewMatrix.transpose(); // Direct3Dでは転置した状態で入れる
		CopyMemory(mappedResource.pData, &viewMatrix.m, sizeof(viewMatrix));
		direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX), 0);

		// プロジェクション行列のマップ
		result = direct3dContext->Map(
			_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 projectionMatrix = Director::getCameraFor2D().getProjectionMatrix();
		projectionMatrix = Mat4::CHIRARITY_CONVERTER * projectionMatrix; // 左手系変換行列はプロジェクション行列に最初からかけておく
		projectionMatrix.transpose();
		CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
		direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

		// デプステクスチャ描画時のプロジェクション行列の情報のマップ
		switch (_renderBufferType) {
			case RenderBufferType::DEPTH_TEXTURE:
			case RenderBufferType::DEPTH_TEXTURE_ORTHOGONAL:
			case RenderBufferType::DEPTH_CUBEMAP_TEXTURE:
			{
				result = direct3dContext->Map(
					_d3dProgram.getConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PARAMETER),
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
				direct3dContext->Unmap(_d3dProgram.getConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PARAMETER), 0);

				result = direct3dContext->Map(
					_d3dProgram.getConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				projectionMatrix = Mat4::CHIRARITY_CONVERTER * _projectionMatrix; // 左手系変換行列はプロジェクション行列に最初からかけておく
				projectionMatrix.transpose();
				CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
				direct3dContext->Unmap(_d3dProgram.getConstantBuffer(CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX), 0);
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
					_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				const Color4F& multiplyColor = Color4F(Color4B(getColor().r, getColor().g, getColor().b, 255));
				CopyMemory(mappedResource.pData, &multiplyColor , sizeof(multiplyColor));
				direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR), 0);
				break;
			}
		}
		
		UINT strides[1] = {sizeof(_quadrangle.topLeft)};
		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgram.getVertexBuffers().size(), _d3dProgram.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgram.getIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgram.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		_d3dProgram.setShadersToDirect3DContext(direct3dContext);
		_d3dProgram.setConstantBuffersToDirect3DContext(direct3dContext);

		UINT startSlot = 0;
		// キューブマップテクスチャは別の
		if (_renderBufferType == RenderBufferType::DEPTH_CUBEMAP_TEXTURE)
		{
			startSlot = 1;
		}

		ID3D11ShaderResourceView* resourceView = _texture->getShaderResourceView(); //TODO:型変換がうまくいかないので一度変数に代入している
		direct3dContext->PSSetShaderResources(startSlot, 1, &resourceView);
		ID3D11SamplerState* samplerState = Director::getRenderer().getLinearSamplerState();
		direct3dContext->PSSetSamplers(0, 1, &samplerState); //TODO:型変換がうまくいかないので一度変数に代入している

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgram.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->DrawIndexed(4, 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		// cocos2d-xはTriangleCommand発行してる形だからな。。テクスチャバインドはTexture2Dでやってるのに大丈夫か？
		glUseProgram(_glProgram.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		// デプステクスチャ描画時のプロジェクション行列のマップ
		switch (_renderBufferType) {
			case RenderBufferType::DEPTH_TEXTURE:
			case RenderBufferType::DEPTH_TEXTURE_ORTHOGONAL:
			case RenderBufferType::DEPTH_CUBEMAP_TEXTURE: //TODO:未対応
			{
				glUniform1f(_glProgram.getUniformLocation("u_nearClipZ"), -Director::getInstance()->getNearClip());
				Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

				glUniform1f(_glProgram.getUniformLocation("u_farClipZ"), -Director::getInstance()->getFarClip());
				Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

				glUniformMatrix4fv(_glProgram.getUniformLocation("u_depthTextureProjectionMatrix"), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
				Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
				break;
			}
			case RenderBufferType::GBUFFER_COLOR_SPECULAR_INTENSITY:
			case RenderBufferType::GBUFFER_NORMAL:
			case RenderBufferType::GBUFFER_SPECULAR_POWER:
				break;
			default:
				glUniform3f(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
				Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
				break;
		}

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.position);
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.textureCoordinate);

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
