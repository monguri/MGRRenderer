#include "Polygon2D.h"
#include "renderer/Director.h"
#include "renderer/Shaders.h"

namespace mgrrenderer
{
bool Polygon2D::initWithVertexArray(const std::vector<Vec2>& vertexArray)
{
	if (vertexArray.size() % 3 != 0)
	{
		// 三角形の頂点のため頂点数が3の倍数であることを前提にする
		return false;
	}

	_vertexArray = vertexArray;

#if defined(MGRRENDERER_USE_DIRECT3D)
	size_t numVertex = _vertexArray.size();

	// 頂点バッファの定義
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vec2) * numVertex;
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
	std::vector<ID3D11Buffer*> oneMeshVBs;
	oneMeshVBs.push_back(vertexBuffer);
	_d3dProgramForForwardRendering.addVertexBuffers(oneMeshVBs);

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

	std::vector<ID3D11Buffer*> indexBufferList;
	indexBufferList.push_back(indexBuffer);

	_d3dProgramForForwardRendering.addIndexBuffers(indexBufferList);

	bool depthEnable = false;
	_d3dProgramForForwardRendering.initWithShaderFile("Resources/shader/PositionMultiplyColor.hlsl", depthEnable, "VS", "", "PS");

	// 入力レイアウトオブジェクトの作成
	D3D11_INPUT_ELEMENT_DESC layout[] = { {D3DProgram::SEMANTIC_POSITION.c_str(), 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0} };
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

	constantBufferDesc.ByteWidth = sizeof(Color4F); // getColor()のColor3Bにすると12バイト境界なので16バイト境界のためにパディングデータを作らねばならない
	// 乗算色
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);
#elif defined(MGRRENDERER_USE_OPENGL)
	_glProgramForForwardRendering.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderPosition.glsl", "../MGRRenderer/Resources/shader/FragmentShaderPositionMultiplyColor.glsl");
#endif

	return true;
}

#if defined(MGRRENDERER_DEFERRED_RENDERING)
void Polygon2D::renderGBuffer()
{
	Node::renderGBuffer();
}
#endif

void Polygon2D::renderForward()
{
	_renderForwardCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();

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

		// メッシュはひとつだけ
		UINT strides[1] = {sizeof(Vec2)};
		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForForwardRendering.getVertexBuffers(0).size(), _d3dProgramForForwardRendering.getVertexBuffers(0).data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForForwardRendering.getIndexBuffer(0, 0), DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForForwardRendering.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForForwardRendering.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForForwardRendering.setConstantBuffersToDirect3DContext(direct3dContext);

		direct3dContext->DrawIndexed(_vertexArray.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForForwardRendering.getShaderProgram());
		GLProgram::checkGLError();

		glUniform3f(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		GLProgram::checkGLError();

		glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getViewMatrix().m);
		glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getProjectionMatrix().m);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();
		glLineWidth(1.0f);
		GLProgram::checkGLError();

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		GLProgram::checkGLError();
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
		GLProgram::checkGLError();
#endif
	});

	Director::getRenderer().addCommand(&_renderForwardCommand);
}

} // namespace mgrrenderer
