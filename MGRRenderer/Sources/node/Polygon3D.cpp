#include "Polygon3D.h"
#include "renderer/Director.h"
#include "Camera.h"
#include "Light.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "renderer/D3DTexture.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "renderer/GLTexture.h"
#include "renderer/Shaders.h"
#endif

namespace mgrrenderer
{
bool Polygon3D::initWithVertexArray(const std::vector<Vec3>& vertexArray)
{
	if (vertexArray.size() % 3 != 0)
	{
		// 三角形の頂点のため頂点数が3の倍数であることを前提にする
		return false;
	}

	_vertexArray = vertexArray;

	_normalArray.clear();

	// TODO:GL_TRIANGLESなので重複した法線の場合はパイプラインでどっちが後に処理されたかによる。そこは考慮していない。考慮する場合はGL_TRIANGLESをそもそも使わない
	size_t numPolygon = vertexArray.size() / 3;
	for (size_t i = 0; i < numPolygon; ++i)
	{
		// 0,1,2と左回りになっているのを前提とする
		Vec3 normal0 = Vec3::cross(vertexArray[3 * i + 1] - vertexArray[3 * i], vertexArray[3 * i + 2] - vertexArray[3 * i]); // 左回りを前提とする
		normal0.normalize();
#if defined(MGRRENDERER_USE_DIRECT3D)
		normal0 = Mat4::CHIRARITY_CONVERTER * normal0;
#endif
		_normalArray.push_back(normal0);

		Vec3 normal1 = Vec3::cross(vertexArray[3 * i + 2] - vertexArray[3 * i + 1], vertexArray[3 * i] - vertexArray[3 * i + 1]);
		normal1.normalize();
#if defined(MGRRENDERER_USE_DIRECT3D)
		normal1 = Mat4::CHIRARITY_CONVERTER * normal1;
#endif
		_normalArray.push_back(normal1);

		Vec3 normal2 = Vec3::cross(vertexArray[3 * i] - vertexArray[3 * i + 2], vertexArray[3 * i + 1] - vertexArray[3 * i + 2]);
		normal2.normalize();
#if defined(MGRRENDERER_USE_DIRECT3D)
		normal2 = Mat4::CHIRARITY_CONVERTER * normal2;
#endif
		_normalArray.push_back(normal2);
	}

#if defined(MGRRENDERER_USE_DIRECT3D)
	size_t numVertex = _vertexArray.size();

	// 頂点バッファの定義
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vec3) * numVertex;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// 頂点バッファのサブリソースの定義
	D3D11_SUBRESOURCE_DATA vertexBufferSubData;
	vertexBufferSubData.pSysMem = _vertexArray.data();
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
	_d3dProgramForShadowMap.addVertexBuffer(vertexBuffer);
	_d3dProgramForPointLightShadowMap.addVertexBuffer(vertexBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addVertexBuffer(vertexBuffer);
#endif

	// ノーマルバッファのサブリソースの作成
	vertexBufferSubData.pSysMem = _normalArray.data();

	result = direct3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubData, &vertexBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addVertexBuffer(vertexBuffer);
	_d3dProgramForShadowMap.addVertexBuffer(vertexBuffer);
	_d3dProgramForPointLightShadowMap.addVertexBuffer(vertexBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addVertexBuffer(vertexBuffer);
#endif

	// インデックスバッファ用の配列の用意。素直に昇順に番号付けする
	std::vector<unsigned int> indexArray;
	indexArray.resize(numVertex);
	for (unsigned int i = 0; i < numVertex; i++)
	{
		indexArray[i] = i;
	}

	// インデックスバッファの定義
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(UINT) * numVertex;
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
	_d3dProgramForForwardRendering.addIndexBuffer(indexBuffer);
	_d3dProgramForShadowMap.addIndexBuffer(indexBuffer);
	_d3dProgramForPointLightShadowMap.addIndexBuffer(indexBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addIndexBuffer(indexBuffer);
#endif

	bool depthEnable = true;
	_d3dProgramForForwardRendering.initWithShaderFile("Resources/shader/Polygon3DForward.hlsl", depthEnable, "VS", "", "PS");
	_d3dProgramForShadowMap.initWithShaderFile("Resources/shader/Polygon3D.hlsl", depthEnable, "VS_SM", "", "");
	_d3dProgramForPointLightShadowMap.initWithShaderFile("Resources/shader/Polygon3D.hlsl", depthEnable, "VS_SM_POINT_LIGHT", "GS_SM_POINT_LIGHT", "");
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.initWithShaderFile("Resources/shader/Polygon3D.hlsl", depthEnable, "VS_GBUFFER", "", "PS_GBUFFER");
#endif

	// 入力レイアウトオブジェクトの作成
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{D3DProgram::SEMANTIC_POSITION.c_str(), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{D3DProgram::SEMANTIC_NORMAL.c_str(), 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
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
	_d3dProgramForShadowMap.setInputLayout(inputLayout);
	_d3dProgramForPointLightShadowMap.setInputLayout(inputLayout);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.setInputLayout(inputLayout);
#endif

	// 定数バッファの作成
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	constantBufferDesc.ByteWidth = sizeof(Mat4);

	// render mode用
	ID3D11Buffer* constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_RENDER_MODE, constantBuffer);

	// Model行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX, constantBuffer);
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX, constantBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX, constantBuffer);
#endif

	// View行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);
#endif

	// Projection行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);
#endif

	// デプスバイアス行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX, constantBuffer);
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX, constantBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX, constantBuffer);
#endif

	// Normal行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer);
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer);
#endif

	// 乗算色
	constantBufferDesc.ByteWidth = sizeof(Color4F); // getColor()のColor3Bにすると12バイト境界なので16バイト境界のためにパディングデータを作らねばならない
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);
#endif

	// アンビエントライトカラー
	constantBufferDesc.ByteWidth = sizeof(AmbientLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer);
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer);

	// ディレクショナルトライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(DirectionalLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer);
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer);

	// ポイントライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(PointLight::ConstantBufferData) * PointLight::MAX_NUM;
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer);

	constantBufferDesc.ByteWidth = sizeof(PointLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer);

	// スポットライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(SpotLight::ConstantBufferData) * SpotLight::MAX_NUM;
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER, constantBuffer);
#elif defined(MGRRENDERER_USE_OPENGL)
	// TODO:objのシェーダとほぼ同じ。共通化したい。
	_glProgramForForwardRendering.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderPositionNormal3D.glsl", "../MGRRenderer/Resources/shader/FragmentShaderPositionNormalMultiplyColor3D.glsl");

	_glProgramForShadowMap.initWithShaderString(
		// vertex shader
		// ModelDataしか使わない場合
		"#version 430\n"
		"attribute vec4 a_position;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_lightViewMatrix;" // 影付けに使うライトをカメラに見立てたビュー行列
		"uniform mat4 u_lightProjectionMatrix;"
		"void main()"
		"{"
		"	gl_Position = u_lightProjectionMatrix * u_lightViewMatrix * u_modelMatrix * a_position;"
		"}"
		,
		// fragment shader
		"#version 430\n"
		"void main()"
		"{" // 何もせずとも深度は自動で書き込まれる 
		"}"
	);

	// STRINGIFYによる読み込みだと、GeForce850Mがうまく#versionの行の改行を読み取ってくれずGLSLコンパイルエラーになる
	_glProgramForGBuffer.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderPositionNormal.glsl", "../MGRRenderer/Resources/shader/FragmentShaderPositionNormalMultiplyColorGBuffer.glsl");
#endif

	return true;
}

#if defined(MGRRENDERER_DEFERRED_RENDERING)
void Polygon3D::renderGBuffer()
{
	_renderGBufferCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();

		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// モデル行列のマップ
		HRESULT result = direct3dContext->Map(
			_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 modelMatrix = getModelMatrix().createTranspose();
		CopyMemory(mappedResource.pData, &modelMatrix.m, sizeof(modelMatrix));
		direct3dContext->Unmap(_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX), 0);

		// ビュー行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 viewMatrix = Director::getCamera().getViewMatrix().createTranspose();
		CopyMemory(mappedResource.pData, &viewMatrix.m, sizeof(viewMatrix));
		direct3dContext->Unmap(_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX), 0);

		// プロジェクション行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 projectionMatrix = (Mat4::CHIRARITY_CONVERTER * Director::getCamera().getProjectionMatrix()).transpose();
		CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
		direct3dContext->Unmap(_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

		// ノーマル行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 normalMatrix = Mat4::CHIRARITY_CONVERTER * Mat4::createNormalMatrix(getModelMatrix());
		normalMatrix.transpose();
		CopyMemory(mappedResource.pData, &normalMatrix.m, sizeof(normalMatrix));
		direct3dContext->Unmap(_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX), 0);

		// 乗算色のマップ
		result = direct3dContext->Map(
			_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		const Color4F& multiplyColor = Color4F(Color4B(getColor().r, getColor().g, getColor().b, 255));
		CopyMemory(mappedResource.pData, &multiplyColor , sizeof(multiplyColor));
		direct3dContext->Unmap(_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR), 0);

		UINT strides[2] = {sizeof(Vec3), sizeof(Vec3)};
		UINT offsets[2] = {0, 0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForGBuffer.getVertexBuffers().size(), _d3dProgramForGBuffer.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForGBuffer.getIndexBuffers()[0], DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForGBuffer.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForGBuffer.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForGBuffer.setConstantBuffersToDirect3DContext(direct3dContext);

		direct3dContext->DrawIndexed(_vertexArray.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		//TODO: GLSL4.0.0を使っているので書き方を変えねばならない
		glUseProgram(_glProgramForGBuffer.getShaderProgram());
		GLProgram::checkGLError();

		glUniform3f(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		GLProgram::checkGLError();

		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		GLProgram::checkGLError();

		Mat4 normalMatrix = Mat4::createNormalMatrix(getModelMatrix());
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_NORMAL_MATRIX), 1, GL_FALSE, (GLfloat*)&normalMatrix.m);

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		GLProgram::checkGLError();

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		GLProgram::checkGLError();
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, sizeof(_normalArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_normalArray.data());
		GLProgram::checkGLError();

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
		GLProgram::checkGLError();
#endif
	});

	Director::getRenderer().addCommand(&_renderGBufferCommand);
}
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

void Polygon3D::renderDirectionalLightShadowMap(const DirectionalLight* light)
{
	Logger::logAssert(light != nullptr, "ライト引数はnullでない前提。");
	Logger::logAssert(light->hasShadowMap(), "ライトはシャドウを持っている前提。");

	_renderDirectionalLightShadowMapCommand.init([=]
	{
		Mat4 lightViewMatrix = light->getShadowMapData().viewMatrix;
		Mat4 lightProjectionMatrix = light->getShadowMapData().projectionMatrix;

#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// モデル行列のマップ
		HRESULT result = direct3dContext->Map(
			_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 modelMatrix = getModelMatrix().createTranspose();
		CopyMemory(mappedResource.pData, &modelMatrix.m, sizeof(modelMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX), 0);

		// ビュー行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		lightViewMatrix.transpose(); // Direct3Dでは転置した状態で入れる
		CopyMemory(mappedResource.pData, &lightViewMatrix.m, sizeof(lightViewMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX), 0);

		// プロジェクション行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		lightProjectionMatrix = Mat4::CHIRARITY_CONVERTER * lightProjectionMatrix; // 左手系変換行列はプロジェクション行列に最初からかけておく
		lightProjectionMatrix.transpose();
		CopyMemory(mappedResource.pData, &lightProjectionMatrix.m, sizeof(lightProjectionMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

		UINT strides[2] = {sizeof(Vec3), sizeof(Vec3)};
		UINT offsets[2] = {0, 0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForShadowMap.getVertexBuffers().size(), _d3dProgramForShadowMap.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForShadowMap.getIndexBuffers()[0], DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForShadowMap.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForShadowMap.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForShadowMap.setConstantBuffersToDirect3DContext(direct3dContext);

		direct3dContext->DrawIndexed(_vertexArray.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		GLProgram::checkGLError();

		glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);

		glUniformMatrix4fv(
			_glProgramForShadowMap.getUniformLocation("u_lightViewMatrix"),
			1,
			GL_FALSE,
			(GLfloat*)lightViewMatrix.m
		);
		GLProgram::checkGLError();

		glUniformMatrix4fv(
			_glProgramForShadowMap.getUniformLocation("u_lightProjectionMatrix"),
			1,
			GL_FALSE,
			(GLfloat*)lightProjectionMatrix.m
		);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		GLProgram::checkGLError();

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		GLProgram::checkGLError();
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, sizeof(_normalArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_normalArray.data());
		GLProgram::checkGLError();

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
		GLProgram::checkGLError();
#endif
	});

	Director::getRenderer().addCommand(&_renderDirectionalLightShadowMapCommand);
}

void Polygon3D::renderPointLightShadowMap(size_t index, const PointLight* light, CubeMapFace face)
{
	Logger::logAssert(light != nullptr, "ライト引数はnullでない前提。");
	Logger::logAssert(light->hasShadowMap(), "ライトはシャドウを持っている前提。");

	_renderPointLightShadowMapCommandList[index][(size_t)face].init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		(void)face;
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// モデル行列のマップ
		HRESULT result = direct3dContext->Map(
			_d3dProgramForPointLightShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 modelMatrix = getModelMatrix().createTranspose();
		CopyMemory(mappedResource.pData, &modelMatrix.m, sizeof(modelMatrix));
		direct3dContext->Unmap(_d3dProgramForPointLightShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX), 0);

		// ビュー行列とプロジェクション行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForPointLightShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		CopyMemory(mappedResource.pData, light->getConstantBufferDataPointer(), sizeof(PointLight::ConstantBufferData));
		direct3dContext->Unmap(_d3dProgramForPointLightShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER), 0);

		UINT strides[2] = {sizeof(Vec3), sizeof(Vec3)};
		UINT offsets[2] = {0, 0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForPointLightShadowMap.getVertexBuffers().size(), _d3dProgramForPointLightShadowMap.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForPointLightShadowMap.getIndexBuffers()[0], DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForPointLightShadowMap.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForPointLightShadowMap.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForPointLightShadowMap.setConstantBuffersToDirect3DContext(direct3dContext);

		direct3dContext->DrawIndexed(_vertexArray.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		GLProgram::checkGLError();

		glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);

		const Mat4& lightViewMatrix = light->getShadowMapData().viewMatrices[(int)face];
		glUniformMatrix4fv(
			_glProgramForShadowMap.getUniformLocation("u_lightViewMatrix"),
			1,
			GL_FALSE,
			(GLfloat*)lightViewMatrix.m
		);
		GLProgram::checkGLError();

		const Mat4& lightProjectionMatrix = light->getShadowMapData().projectionMatrix;
		glUniformMatrix4fv(
			_glProgramForShadowMap.getUniformLocation("u_lightProjectionMatrix"),
			1,
			GL_FALSE,
			(GLfloat*)lightProjectionMatrix.m
		);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		GLProgram::checkGLError();

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		GLProgram::checkGLError();
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, sizeof(_normalArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_normalArray.data());
		GLProgram::checkGLError();

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
		GLProgram::checkGLError();
#endif
	});

	Director::getRenderer().addCommand(&_renderPointLightShadowMapCommandList[index][(size_t)face]);
}

void Polygon3D::renderSpotLightShadowMap(size_t index, const SpotLight* light)
{
	Logger::logAssert(light != nullptr, "ライト引数はnullでない前提。");
	Logger::logAssert(light->hasShadowMap(), "ライトはシャドウを持っている前提。");

	_renderSpotLightShadowMapCommandList[index].init([=]
	{
		Mat4 lightViewMatrix = light->getShadowMapData().viewMatrix;
		Mat4 lightProjectionMatrix = light->getShadowMapData().projectionMatrix;

#if defined(MGRRENDERER_USE_DIRECT3D)
		//TODO:DirectionalLightと全く同じ処理内容。共通化したい
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		
		// モデル行列のマップ
		HRESULT result = direct3dContext->Map(
			_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 modelMatrix = getModelMatrix().createTranspose();
		CopyMemory(mappedResource.pData, &modelMatrix.m, sizeof(modelMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX), 0);

		// ビュー行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		lightViewMatrix.transpose(); // Direct3Dでは転置した状態で入れる
		CopyMemory(mappedResource.pData, &lightViewMatrix.m, sizeof(lightViewMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX), 0);

		// プロジェクション行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		lightProjectionMatrix = Mat4::CHIRARITY_CONVERTER * lightProjectionMatrix; // 左手系変換行列はプロジェクション行列に最初からかけておく
		lightProjectionMatrix.transpose();
		CopyMemory(mappedResource.pData, &lightProjectionMatrix.m, sizeof(lightProjectionMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

		UINT strides[2] = {sizeof(Vec3), sizeof(Vec3)};
		UINT offsets[2] = {0, 0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForShadowMap.getVertexBuffers().size(), _d3dProgramForShadowMap.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForShadowMap.getIndexBuffers()[0], DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForShadowMap.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForShadowMap.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForShadowMap.setConstantBuffersToDirect3DContext(direct3dContext);

		direct3dContext->DrawIndexed(_vertexArray.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		//TODO:DirectionalLightと全く同じ処理内容。共通化したい
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		GLProgram::checkGLError();

		glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);

		glUniformMatrix4fv(
			_glProgramForShadowMap.getUniformLocation("u_lightViewMatrix"),
			1,
			GL_FALSE,
			(GLfloat*)lightViewMatrix.m
		);
		GLProgram::checkGLError();

		glUniformMatrix4fv(
			_glProgramForShadowMap.getUniformLocation("u_lightProjectionMatrix"),
			1,
			GL_FALSE,
			(GLfloat*)lightProjectionMatrix.m
		);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		GLProgram::checkGLError();

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		GLProgram::checkGLError();
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, sizeof(_normalArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_normalArray.data());
		GLProgram::checkGLError();

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
		GLProgram::checkGLError();
#endif
	});

	Director::getRenderer().addCommand(&_renderSpotLightShadowMapCommandList[index]);
}

void Polygon3D::renderForward()
{
	_renderForwardCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();

		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// render modeのマップ
		HRESULT result = direct3dContext->Map(
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_RENDER_MODE),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Renderer::RenderMode renderMode = Director::getRenderer().getRenderMode();
		CopyMemory(mappedResource.pData, &renderMode, sizeof(renderMode));
		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_RENDER_MODE), 0);

		// モデル行列のマップ
		result = direct3dContext->Map(
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
		Mat4 viewMatrix = Director::getCamera().getViewMatrix().createTranspose();
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
		Mat4 projectionMatrix = (Mat4::CHIRARITY_CONVERTER * Director::getCamera().getProjectionMatrix()).transpose();
		CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

		// デプスバイアス行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 depthBiasMatrix = (Mat4::TEXTURE_COORDINATE_CONVERTER * Mat4::createScale(Vec3(0.5f, 0.5f, 1.0f)) * Mat4::createTranslation(Vec3(1.0f, -1.0f, 0.0f))).transpose(); //TODO: Mat4を参照型にすると値がおかしくなってしまう
		CopyMemory(mappedResource.pData, &depthBiasMatrix.m, sizeof(depthBiasMatrix));
		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX), 0);

		// ノーマル行列のマップ
		result = direct3dContext->Map(
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 normalMatrix = Mat4::CHIRARITY_CONVERTER * Mat4::createNormalMatrix(getModelMatrix());
		normalMatrix.transpose();
		CopyMemory(mappedResource.pData, &normalMatrix.m, sizeof(normalMatrix));
		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX), 0);

		// 乗算色のマップ
		result = direct3dContext->Map(
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		const Color4F& multiplyColor = Color4F(Color3B(getColor().r, getColor().g, getColor().b), getOpacity());
		CopyMemory(mappedResource.pData, &multiplyColor , sizeof(multiplyColor));
		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR), 0);

		const Scene& scene = Director::getInstance()->getScene();


		const AmbientLight* ambientLight = scene.getAmbientLight();
		Logger::logAssert(ambientLight != nullptr, "シーンにアンビエントライトがない。");

		// アンビエントライトカラーのマップ
		result = direct3dContext->Map(
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		CopyMemory(mappedResource.pData, ambientLight->getConstantBufferDataPointer(), sizeof(AmbientLight::ConstantBufferData));
		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER), 0);


		// ディレクショナルライト
		ID3D11ShaderResourceView* dirLightShadowMapResourceView = nullptr;
		const DirectionalLight* directionalLight = scene.getDirectionalLight();
		// 光の方向に向けてシャドウマップを作るカメラが向いていると考え、カメラから見たモデル座標系にする
		if (directionalLight != nullptr)
		{
			if (directionalLight->hasShadowMap())
			{
				ID3D11ShaderResourceView* shaderResouceView[1] = { directionalLight->getShadowMapData().depthTexture->getShaderResourceView() };
				direct3dContext->PSSetShaderResources(0, 1, shaderResouceView);
			}

			result = direct3dContext->Map(
				_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
			CopyMemory(mappedResource.pData, directionalLight->getConstantBufferDataPointer(), sizeof(DirectionalLight::ConstantBufferData));
			direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER), 0);

			dirLightShadowMapResourceView = directionalLight->getShadowMapData().depthTexture->getShaderResourceView();
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
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER),
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

		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER), 0);


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
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER),
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

		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER), 0);


		UINT strides[2] = {sizeof(Vec3), sizeof(Vec3)};
		UINT offsets[2] = {0, 0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForForwardRendering.getVertexBuffers().size(), _d3dProgramForForwardRendering.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForForwardRendering.getIndexBuffers()[0], DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForForwardRendering.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForForwardRendering.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForForwardRendering.setConstantBuffersToDirect3DContext(direct3dContext);

		ID3D11ShaderResourceView* shaderResourceViews[1] = {
			dirLightShadowMapResourceView,
		};
		direct3dContext->PSSetShaderResources(0, 1, shaderResourceViews);

		direct3dContext->PSSetShaderResources(1, pointLightShadowCubeMapResourceView.size(), pointLightShadowCubeMapResourceView.data());

		direct3dContext->PSSetShaderResources(1 + pointLightShadowCubeMapResourceView.size(), spotLightShadowMapResourceView.size(), spotLightShadowMapResourceView.data());

		ID3D11SamplerState* samplerState[1] = { Director::getRenderer().getPCFSamplerState() };
		direct3dContext->PSSetSamplers(0, 1, samplerState);

		direct3dContext->DrawIndexed(_vertexArray.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForForwardRendering.getShaderProgram());
		GLProgram::checkGLError();

		glUniform1i(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_RENDER_MODE), (GLint)Director::getRenderer().getRenderMode());

		glUniform4f(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f, getOpacity());
		GLProgram::checkGLError();

		glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		GLProgram::checkGLError();

		Mat4 normalMatrix = Mat4::createNormalMatrix(getModelMatrix());
		glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_NORMAL_MATRIX), 1, GL_FALSE, (GLfloat*)&normalMatrix.m);

		static const Mat4& depthBiasMatrix = Mat4::createScale(Vec3(0.5f, 0.5f, 0.5f)) * Mat4::createTranslation(Vec3(1.0f, 1.0f, 1.0f));
		glUniformMatrix4fv(
			_glProgramForForwardRendering.getUniformLocation("u_depthBiasMatrix"),
			1,
			GL_FALSE,
			(GLfloat*)depthBiasMatrix.m
		);
		GLProgram::checkGLError();

		// アンビエントライト
		const Scene& scene = Director::getInstance()->getScene();
		const AmbientLight* ambientLight = scene.getAmbientLight();
		Logger::logAssert(ambientLight != nullptr, "シーンにアンビエントライトがない。");
		Color3B lightColor = ambientLight->getColor();
		float intensity = ambientLight->getIntensity();
		glUniform3f(_glProgramForForwardRendering.getUniformLocation("u_ambientLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
		GLProgram::checkGLError();


		// ディレクショナルライト
		const DirectionalLight* directionalLight = scene.getDirectionalLight();
		// 光の方向に向けてシャドウマップを作るカメラが向いていると考え、カメラから見たモデル座標系にする
		if (directionalLight != nullptr)
		{
			glUniform1i(
				_glProgramForForwardRendering.getUniformLocation("u_directionalLightIsValid"),
				1
			);
			GLProgram::checkGLError();

			lightColor = directionalLight->getColor();
			intensity = directionalLight->getIntensity();
			glUniform3f(_glProgramForForwardRendering.getUniformLocation("u_directionalLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
			GLProgram::checkGLError();

			Vec3 direction = directionalLight->getDirection();
			direction.normalize();
			glUniform3fv(_glProgramForForwardRendering.getUniformLocation("u_directionalLightDirection"), 1, (GLfloat*)&direction);
			GLProgram::checkGLError();

			if (directionalLight->hasShadowMap())
			{
				glUniformMatrix4fv(
					_glProgramForForwardRendering.getUniformLocation("u_directionalLightViewMatrix"),
					1,
					GL_FALSE,
					(GLfloat*)directionalLight->getShadowMapData().viewMatrix.m
				);

				glUniformMatrix4fv(
					_glProgramForForwardRendering.getUniformLocation("u_directionalLightProjectionMatrix"),
					1,
					GL_FALSE,
					(GLfloat*)directionalLight->getShadowMapData().projectionMatrix.m
				);

				glActiveTexture(GL_TEXTURE0);
				GLuint textureId = directionalLight->getShadowMapData().getDepthTexture()->getTextureId();
				glBindTexture(GL_TEXTURE_2D, textureId);
				glUniform1i(_glProgramForForwardRendering.getUniformLocation("u_directionalLightShadowMap"), 0);
			}
		}

		// ポイントライト
		for (size_t i = 0; i < PointLight::MAX_NUM; i++)
		{
			const PointLight* pointLight = scene.getPointLight(i);
			if (pointLight != nullptr)
			{
				glUniform1i(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_pointLightIsValid[") + std::to_string(i) + std::string("]")).c_str()), 1);
				//glUniform1f(_glProgramForForwardRendering.getUniformLocation(std::string("u_pointLightRangeInverse[") + std::to_string(i) + std::string("]")), 1.0f / pointLight->getRange());
				GLProgram::checkGLError();

				lightColor = pointLight->getColor();
				intensity = pointLight->getIntensity();
				glUniform3f(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_pointLightColor[") + std::to_string(i) + std::string("]")).c_str()), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
				//glUniform3f(_glProgramForForwardRendering.getUniformLocation(std::string("u_pointLightColor[") + std::to_string(i) + std::string("]")), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);

				GLProgram::checkGLError();

				glUniform3fv(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_pointLightPosition[") + std::to_string(i) + std::string("]")).c_str()), 1, (GLfloat*)&pointLight->getPosition()); // ライトについてはローカル座標でなくワールド座標である前提
				//glUniform3fv(_glProgramForForwardRendering.getUniformLocation(std::string("u_pointLightPosition[") + std::to_string(i) + std::string("]")), 1, (GLfloat*)&pointLight->getPosition()); // ライトについてはローカル座標でなくワールド座標である前提
				GLProgram::checkGLError();

				glUniform1f(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_pointLightRangeInverse[") + std::to_string(i) + std::string("]")).c_str()), 1.0f / pointLight->getRange());
				//glUniform1f(_glProgramForForwardRendering.getUniformLocation(std::string("u_pointLightRangeInverse[") + std::to_string(i) + std::string("]")), 1.0f / pointLight->getRange());
				GLProgram::checkGLError();

				glUniform1i(
					glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_pointLightHasShadowMap[") + std::to_string(i) + std::string("]")).c_str()),
					pointLight->hasShadowMap()
				);
				//glUniform1i(
				//	_glProgramForForwardRendering.getUniformLocation(std::string("u_pointLightHasShadowMap[") + std::to_string(i) + std::string("]")),
				//	pointLight->hasShadowMap()
				//);

				if (pointLight->hasShadowMap())
				{
					glUniformMatrix4fv(
						glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_pointLightProjectionMatrix[") + std::to_string(i) + std::string("]")).c_str()),
						1,
						GL_FALSE,
						(GLfloat*)pointLight->getShadowMapData().projectionMatrix.m
					);
					//glUniformMatrix4fv(
					//	_glProgramForForwardRendering.getUniformLocation(std::string("u_pointLightProjectionMatrix[") + std::to_string(i) + std::string("]")),
					//	1,
					//	GL_FALSE,
					//	(GLfloat*)pointLight->getShadowMapData().projectionMatrix.m
					//);

					glActiveTexture(GL_TEXTURE1 + i);
					GLuint textureId = pointLight->getShadowMapData().getDepthTexture()->getTextureId();
					glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
					glUniform1i(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_pointLightShadowCubeMap[") + std::to_string(i) + std::string("]")).c_str()), 1 + i);
					//glUniform1i(_glProgramForForwardRendering.getUniformLocation(std::string("u_pointLightShadowCubeMap[") + std::to_string(i) + std::string("]")), 5 + i);
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
				glUniform1i(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightIsValid[") + std::to_string(i) + std::string("]")).c_str()), 1);
				GLProgram::checkGLError();

				lightColor = spotLight->getColor();
				intensity = spotLight->getIntensity();
				glUniform3f(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightColor[") + std::to_string(i) + std::string("]")).c_str()), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
				//glUniform3f(_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightColor[") + std::to_string(i) + std::string("]")), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
				GLProgram::checkGLError();

				glUniform3fv(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightPosition[") + std::to_string(i) + std::string("]")).c_str()), 1, (GLfloat*)&spotLight->getPosition());
				//glUniform3fv(_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightPosition[") + std::to_string(i) + std::string("]")), 1, (GLfloat*)&spotLight->getPosition());
				GLProgram::checkGLError();

				Vec3 direction = spotLight->getDirection();
				direction.normalize();
				glUniform3fv(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightDirection[") + std::to_string(i) + std::string("]")).c_str()), 1, (GLfloat*)&direction);
				//glUniform3fv(_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightDirection[") + std::to_string(i) + std::string("]")), 1, (GLfloat*)&direction);
				GLProgram::checkGLError();

				glUniform1f(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightRangeInverse[") + std::to_string(i) + std::string("]")).c_str()), 1.0f / spotLight->getRange());
				//glUniform1f(_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightRangeInverse[") + std::to_string(i) + std::string("]")), 1.0f / spotLight->getRange());
				GLProgram::checkGLError();

				glUniform1f(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightInnerAngleCos[") + std::to_string(i) + std::string("]")).c_str()), spotLight->getInnerAngleCos());
				//glUniform1f(_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightInnerAngleCos[") + std::to_string(i) + std::string("]")), spotLight->getInnerAngleCos());
				GLProgram::checkGLError();

				glUniform1f(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightOuterAngleCos[") + std::to_string(i) + std::string("]")).c_str()), spotLight->getOuterAngleCos());
				//glUniform1f(_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightOuterAngleCos[") + std::to_string(i) + std::string("]")), spotLight->getOuterAngleCos());
				GLProgram::checkGLError();

				glUniform1i(
					glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightHasShadowMap[") + std::to_string(i) + std::string("]")).c_str()),
					spotLight->hasShadowMap()
				);
				//glUniform1i(
				//	_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightHasShadowMap[") + std::to_string(i) + std::string("]")),
				//	spotLight->hasShadowMap()
				//);

				if (spotLight->hasShadowMap())
				{
					glUniformMatrix4fv(
						glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightViewMatrix[") + std::to_string(i) + std::string("]")).c_str()),
						1,
						GL_FALSE,
						(GLfloat*)spotLight->getShadowMapData().viewMatrix.m
					);
					//glUniformMatrix4fv(
					//	_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightViewMatrix[") + std::to_string(i) + std::string("]")),
					//	1,
					//	GL_FALSE,
					//	(GLfloat*)spotLight->getShadowMapData().viewMatrix.m
					//);

					glUniformMatrix4fv(
						glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightProjectionMatrix[") + std::to_string(i) + std::string("]")).c_str()),
						1,
						GL_FALSE,
						(GLfloat*)spotLight->getShadowMapData().projectionMatrix.m
					);
					//glUniformMatrix4fv(
					//	_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightProjectionMatrix[") + std::to_string(i) + std::string("]")),
					//	1,
					//	GL_FALSE,
					//	(GLfloat*)spotLight->getShadowMapData().projectionMatrix.m
					//);

					glActiveTexture(GL_TEXTURE5 + i);
					GLuint textureId = spotLight->getShadowMapData().getDepthTexture()->getTextureId();
					glBindTexture(GL_TEXTURE_2D, textureId);
					glUniform1i(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightShadowMap[") + std::to_string(i) + std::string("]")).c_str()), 5 + i);
					//glUniform1i(_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightShadowMap[") + std::to_string(i) + std::string("]")), 9 + i);
					glActiveTexture(GL_TEXTURE0);
				}
			}
		}

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		GLProgram::checkGLError();

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		GLProgram::checkGLError();
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, sizeof(_normalArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_normalArray.data());
		GLProgram::checkGLError();

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
		GLProgram::checkGLError();
#endif
	});

	Director::getRenderer().addCommand(&_renderForwardCommand);
}

} // namespace mgrrenderer
