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
	ID3D11Device* direct3dDevice = Director::getInstance()->getDirect3dDevice();
	ID3D11Buffer* vertexBuffer = nullptr;
	HRESULT result = direct3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubData, &vertexBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addVertexBuffer(vertexBuffer);
	_d3dProgramForShadowMap.addVertexBuffer(vertexBuffer);
	_d3dProgramForGBuffer.addVertexBuffer(vertexBuffer);

	// ノーマルバッファのサブリソースの作成
	vertexBufferSubData.pSysMem = _normalArray.data();

	result = direct3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubData, &vertexBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addVertexBuffer(vertexBuffer);
	_d3dProgramForShadowMap.addVertexBuffer(vertexBuffer);
	_d3dProgramForGBuffer.addVertexBuffer(vertexBuffer);

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
	_d3dProgram.setIndexBuffer(indexBuffer);
	_d3dProgramForShadowMap.setIndexBuffer(indexBuffer);
	_d3dProgramForGBuffer.setIndexBuffer(indexBuffer);

	bool depthEnable = true;
	_d3dProgram.initWithShaderFile("Resources/shader/Polygon3D.hlsl", depthEnable, "VS", "", "PS");
	_d3dProgramForShadowMap.initWithShaderFile("Resources/shader/Polygon3D.hlsl", depthEnable, "VS_SM", "", "");
	_d3dProgramForGBuffer.initWithShaderFile("Resources/shader/Polygon3D.hlsl", depthEnable, "VS_GBUFFER", "", "PS_GBUFFER");

	// 入力レイアウトオブジェクトの作成
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{D3DProgram::SEMANTIC_POSITION.c_str(), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{D3DProgram::SEMANTIC_NORMAL.c_str(), 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
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
	_d3dProgramForShadowMap.setInputLayout(inputLayout);
	_d3dProgramForGBuffer.setInputLayout(inputLayout);

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
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX, constantBuffer);
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX, constantBuffer);

	// View行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);

	// Projection行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);

	// Normal行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer);
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer);

	// 乗算色
	constantBufferDesc.ByteWidth = sizeof(Color4F); // getColor()のColor3Bにすると12バイト境界なので16バイト境界のためにパディングデータを作らねばならない
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);

	// アンビエントライトカラー
	constantBufferDesc.ByteWidth = sizeof(AmbientLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer);

	// ディレクショナルトライトView行列用
	constantBufferDesc.ByteWidth = sizeof(Mat4);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX, constantBuffer);

	// ディレクショナルトライトProjection行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX, constantBuffer);

	// ディレクショナルトライトデプスバイアス行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_DEPTH_BIAS_MATRIX, constantBuffer);

	// ディレクショナルトライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(DirectionalLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer);

	// ポイントライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(PointLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer);

	// スポットライトパラメーター
	constantBufferDesc.ByteWidth = sizeof(SpotLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER, constantBuffer);
#elif defined(MGRRENDERER_USE_OPENGL)
	// TODO:objのシェーダとほぼ同じ。共通化したい。
	_glProgram.initWithShaderString(
		// vertex shader
		"attribute vec4 a_position;"
		"attribute vec4 a_normal;"
		"varying vec4 v_normal;"
		"varying vec4 v_lightPosition;"
		"varying vec3 v_vertexToPointLightDirection;"
		"varying vec3 v_vertexToSpotLightDirection;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_viewMatrix;"
		"uniform mat4 u_lightViewMatrix;" // 影付けに使うライトをカメラに見立てたビュー行列
		"uniform mat4 u_lightProjectionMatrix;" // 影付けに使うライトをカメラに見立てたプロジェクション行列
		"uniform mat4 u_projectionMatrix;"
		"uniform mat4 u_depthBiasMatrix;"
		"uniform mat4 u_normalMatrix;"
		"uniform vec3 u_pointLightPosition;"
		"uniform vec3 u_spotLightPosition;"
		"void main()"
		"{"
		"	vec4 worldPosition = u_modelMatrix * a_position;"
		"	v_vertexToPointLightDirection = u_pointLightPosition - worldPosition.xyz;"
		"	v_vertexToSpotLightDirection = u_spotLightPosition - worldPosition.xyz;"
		"	gl_Position = u_projectionMatrix * u_viewMatrix * worldPosition;"
		"	v_normal = vec4(normalize((u_normalMatrix * a_normal).xyz), 1.0);" // scale変換に対応するためにモデル行列の逆行列を転置したものを用いる
		"	v_lightPosition = u_depthBiasMatrix * u_lightProjectionMatrix * u_lightViewMatrix * worldPosition;"
		"}"
		,
		// fragment shader
		"uniform sampler2DShadow u_shadowTexture;"
		"uniform vec3 u_multipleColor;"
		"uniform vec3 u_ambientLightColor;"
		"uniform vec3 u_directionalLightColor;"
		"uniform vec3 u_directionalLightDirection;"
		"uniform vec3 u_pointLightColor;"
		"uniform float u_pointLightRangeInverse;"
		"uniform vec3 u_spotLightColor;"
		"uniform vec3 u_spotLightDirection;"
		"uniform float u_spotLightRangeInverse;"
		"uniform float u_spotLightInnerAngleCos;"
		"uniform float u_spotLightOuterAngleCos;"
		"varying vec4 v_normal;"
		"varying vec3 v_vertexToPointLightDirection;"
		"varying vec3 v_vertexToSpotLightDirection;"
		"varying vec4 v_lightPosition;"
		"vec3 computeLightedColor(vec3 normalVector, vec3 lightDirection, vec3 lightColor, float attenuation)"
		"{"
		"	float diffuse = max(dot(normalVector, lightDirection), 0.0);"
		"	vec3 diffuseColor = lightColor * diffuse * attenuation;"
		"	return diffuseColor;"
		"}"
		""
		"void main()"
		"{"
		"	vec4 ambientLightColor = vec4(u_ambientLightColor, 1.0);"
		""
		"	vec3 normal = normalize(v_normal.xyz);" // データ形式の時点でnormalizeされてない法線がある模様
		"	vec4 diffuseSpecularLightColor = vec4(0.0, 0.0, 0.0, 1.0);"
		"	diffuseSpecularLightColor.rgb += computeLightedColor(normal, -u_directionalLightDirection, u_directionalLightColor, 1.0);"
		""
		"	vec3 dir = v_vertexToPointLightDirection * u_pointLightRangeInverse;"
		"	float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);"
		"	diffuseSpecularLightColor.rgb += computeLightedColor(normal, normalize(v_vertexToPointLightDirection), u_pointLightColor, attenuation);"
		""
		"	dir = v_vertexToSpotLightDirection * u_spotLightRangeInverse;"
		"	attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);"
		"	vec3 vertexToSpotLightDirection = normalize(v_vertexToSpotLightDirection);"
		"	float spotCurrentAngleCos = dot(u_spotLightDirection, -vertexToSpotLightDirection);"
		"	attenuation *= smoothstep(u_spotLightOuterAngleCos, u_spotLightInnerAngleCos, spotCurrentAngleCos);"
		"	attenuation = clamp(attenuation, 0.0, 1.0);"
		"	diffuseSpecularLightColor.rgb += computeLightedColor(normal, vertexToSpotLightDirection, u_spotLightColor, attenuation);"
		""
		"	float outShadowFlag = textureProj(u_shadowTexture, v_lightPosition);"
		"	gl_FragColor = vec4(u_multipleColor, 1.0) * vec4((diffuseSpecularLightColor.rgb * outShadowFlag + ambientLightColor.rgb), 1.0);" // テクスチャ番号は0のみに対応
		"}"
	);

	_glProgramForShadowMap.initWithShaderString(
		// vertex shader
		// ModelDataしか使わない場合
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
		"void main()"
		"{" // 何もせずとも深度は自動で書き込まれる 
		"}"
	);

	// STRINGIFYによる読み込みだと、GeForce850Mがうまく#versionの行の改行を読み取ってくれずGLSLコンパイルエラーになる
	_glProgramForGBuffer.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderPositionNormal.glsl", "../MGRRenderer/Resources/shader/FragmentShaderPositionNormalMultiplyColorGBuffer.glsl");
#endif

	return true;
}

void Polygon3D::renderGBuffer()
{
	_renderGBufferCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();

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
		Mat4 modelMatrix = getModelMatrix();
		modelMatrix.transpose();
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
		Mat4 viewMatrix = Director::getCamera().getViewMatrix();
		viewMatrix.transpose(); // Direct3Dでは転置した状態で入れる
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
		Mat4 projectionMatrix = Director::getCamera().getProjectionMatrix();
		projectionMatrix = Mat4::CHIRARITY_CONVERTER * projectionMatrix; // 左手系変換行列はプロジェクション行列に最初からかけておく
		projectionMatrix.transpose();
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
		Mat4 normalMatrix = Mat4::createNormalMatrix(getModelMatrix());
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
		direct3dContext->IASetIndexBuffer(_d3dProgramForGBuffer.getIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForGBuffer.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForGBuffer.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForGBuffer.setConstantBuffersToDirect3DContext(direct3dContext);

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgramForGBuffer.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->DrawIndexed(_vertexArray.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		//TODO: GLSL4.0.0を使っているので書き方を変えねばならない
		glUseProgram(_glProgramForGBuffer.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniform3f(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		const Mat4& normalMatrix = Mat4::createNormalMatrix(getModelMatrix());
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_NORMAL_MATRIX), 1, GL_FALSE, (GLfloat*)&normalMatrix.m);

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, sizeof(_normalArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_normalArray.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
#endif
	});

	Director::getRenderer().addCommand(&_renderGBufferCommand);
}

void Polygon3D::renderShadowMap()
{
	_renderShadowMapCommand.init([=]
	{
		bool makeShadowMap = false;
		DirectionalLight::ShadowMapData shadowMapData;

		for (Light* light : Director::getLight())
		{
			switch (light->getLightType())
			{
			case LightType::AMBIENT:
				break;
			case LightType::DIRECTION: {
				DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
				// TODO:とりあえず影つけはDirectionalLightのみを想定
				// 光の方向に向けてシャドウマップを作るカメラが向いていると考え、カメラから見たモデル座標系にする
				if (dirLight->hasShadowMap())
				{
					makeShadowMap = true;
					shadowMapData = dirLight->getShadowMapData();
				}
			}
				break;
			case LightType::POINT: {
			}
				break;
			case LightType::SPOT: {
			}
			default:
				break;
			}
		}

		if (!makeShadowMap)
		{
			// シャドウマップを必要とするライトがなければ何もしない
			return;
		}

#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();

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
		Mat4 modelMatrix = getModelMatrix();
		modelMatrix.transpose();
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
		Mat4 lightViewMatrix = shadowMapData.viewMatrix;
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
		Mat4 lightProjectionMatrix = shadowMapData.projectionMatrix;
		lightProjectionMatrix = Mat4::CHIRARITY_CONVERTER * lightProjectionMatrix; // 左手系変換行列はプロジェクション行列に最初からかけておく
		lightProjectionMatrix.transpose();
		CopyMemory(mappedResource.pData, &lightProjectionMatrix.m, sizeof(lightProjectionMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

		UINT strides[2] = {sizeof(Vec3), sizeof(Vec3)};
		UINT offsets[2] = {0, 0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForShadowMap.getVertexBuffers().size(), _d3dProgramForShadowMap.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForShadowMap.getIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForShadowMap.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForShadowMap.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForShadowMap.setConstantBuffersToDirect3DContext(direct3dContext);

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgramForShadowMap.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->DrawIndexed(_vertexArray.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(
			_glProgramForShadowMap.getUniformLocation("u_lightViewMatrix"),
			1,
			GL_FALSE,
			(GLfloat*)shadowMapData.viewMatrix.m
		);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		// TODO:Vec3やMat4に頭につける-演算子作らないと
		glUniformMatrix4fv(
			_glProgramForShadowMap.getUniformLocation("u_lightProjectionMatrix"),
			1,
			GL_FALSE,
			(GLfloat*)shadowMapData.projectionMatrix.m
		);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, sizeof(_normalArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_normalArray.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
#endif
	});

	Director::getRenderer().addCommand(&_renderShadowMapCommand);
}

void Polygon3D::renderForward()
{
	_renderCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();

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
		Mat4 viewMatrix = Director::getCamera().getViewMatrix();
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
		Mat4 projectionMatrix = Director::getCamera().getProjectionMatrix();
		projectionMatrix = Mat4::CHIRARITY_CONVERTER * projectionMatrix; // 左手系変換行列はプロジェクション行列に最初からかけておく
		projectionMatrix.transpose();
		CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
		direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

		// ノーマル行列のマップ
		result = direct3dContext->Map(
			_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 normalMatrix = Mat4::createNormalMatrix(getModelMatrix());
		normalMatrix.transpose();
		CopyMemory(mappedResource.pData, &normalMatrix.m, sizeof(normalMatrix));
		direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX), 0);

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
				DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);

				// TODO:とりあえず影つけはDirectionalLightのみを想定
				// 光の方向に向けてシャドウマップを作るカメラが向いていると考え、カメラから見たモデル座標系にする
				if (dirLight->hasShadowMap())
				{
					result = direct3dContext->Map(
						_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX),
						0,
						D3D11_MAP_WRITE_DISCARD,
						0,
						&mappedResource
					);
					Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
					Mat4 lightViewMatrix = dirLight->getShadowMapData().viewMatrix;
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
					Mat4 lightProjectionMatrix = dirLight->getShadowMapData().projectionMatrix;
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

					ID3D11SamplerState* samplerState = Director::getRenderer().getPCFSamplerState();
					direct3dContext->PSSetSamplers(0, 1, &samplerState); //TODO:型変換がうまくいかないので一度変数に代入している
					ID3D11ShaderResourceView* shaderResouceView = dirLight->getShadowMapData().depthTexture->getShaderResourceView();
					direct3dContext->PSSetShaderResources(0, 1, &shaderResouceView);
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

		UINT strides[2] = {sizeof(Vec3), sizeof(Vec3)};
		UINT offsets[2] = {0, 0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgram.getVertexBuffers().size(), _d3dProgram.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgram.getIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgram.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgram.setShadersToDirect3DContext(direct3dContext);
		_d3dProgram.setConstantBuffersToDirect3DContext(direct3dContext);

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgram.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->DrawIndexed(_vertexArray.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgram.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniform3f(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		const Mat4& normalMatrix = Mat4::createNormalMatrix(getModelMatrix());
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_NORMAL_MATRIX), 1, GL_FALSE, (GLfloat*)&normalMatrix.m);

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

				// TODO:とりあえず影つけはDirectionalLightのみを想定
				// 光の方向に向けてシャドウマップを作るカメラが向いていると考え、カメラから見たモデル座標系にする
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
					// TODO:Vec3やMat4に頭につける-演算子作らないと

					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, dirLight->getShadowMapData().getDepthTexture()->getTextureId());
					glUniform1i(_glProgram.getUniformLocation("u_shadowTexture"), 1);
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
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, sizeof(_normalArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_normalArray.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
#endif
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
