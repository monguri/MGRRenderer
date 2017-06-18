#include "Sprite3D.h"
#include "loader/ObjLoader.h"
#include "renderer/Image.h"
#include "renderer/Director.h"
#include "Light.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "renderer/D3DTexture.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "renderer/GLTexture.h"
#endif

namespace mgrrenderer
{

Sprite3D::Sprite3D() :
_isObj(false),
_isC3b(false),
_useMtl(true),
_meshDatas(nullptr),
_nodeDatas(nullptr),
_perVertexByteSize(0),
_animationDatas(nullptr),
_currentAnimation(nullptr),
_loopAnimation(false),
_elapsedTime(0.0f)
{
}

Sprite3D::~Sprite3D()
{
	_currentAnimation = nullptr;

	if (_animationDatas != nullptr)
	{
		delete _animationDatas;
		_animationDatas = nullptr;
	}

	if (_nodeDatas != nullptr)
	{
		delete _nodeDatas;
		_nodeDatas = nullptr;
	}

	if (_meshDatas != nullptr)
	{
		delete _meshDatas;
		_meshDatas = nullptr;
	}

#if defined(MGRRENDERER_USE_OPENGL)
	glBindTexture(GL_TEXTURE_2D, 0);
#endif

#if defined(MGRRENDERER_USE_DIRECT3D)
	for (D3DTexture* texture : _textureList)
#elif defined(MGRRENDERER_USE_OPENGL)
	for (GLTexture* texture : _textureList)
#endif
	{
		if (texture != nullptr)
		{
			delete texture;
		}
	}

	_textureList.clear();
}

bool Sprite3D::initWithModel(const std::string& filePath, bool useMtl)
{
	Logger::logAssert(_textureList.empty(), "Sprite3DではとりあえずinitWithModelは一回しか呼ばれない前提。");
	Logger::logAssert(_verticesList.empty(), "Sprite3DではとりあえずinitWithModelは一回しか呼ばれない前提。");
	Logger::logAssert(_indicesList.empty(), "Sprite3DではとりあえずinitWithModelは一回しか呼ばれない前提。");

	_isObj = false;
	_isC3b = false;

	const std::string& ext = filePath.substr(filePath.length() - 4, 4);
	if (ext == ".obj")
	{
		_isObj = true;
		_useMtl = useMtl;

		std::vector<ObjLoader::MeshData> meshList;
		std::vector<ObjLoader::MaterialData> materialList;

		const std::string& err = ObjLoader::loadObj(filePath, meshList, materialList);
		if (!err.empty())
		{
			Logger::log(err.c_str());
			return false;
		}

		// materialListは現状無視
		// TODO:そもそもMeshDataはその時点でマテリアルごとにまとまってないのでは？faceGroupだからまとまってる？
		// →まとまってる。しかし、今回はマテリアルは一種類という前提でいこう
		// 本来は、std::vector<std::vector<Position3DTextureCoordinates>> がメンバ変数になってて、マテリアルごとに切り替えて描画する
		// テクスチャも本来は切り替え前提だからsetTextureってメソッドおかしいよな。。
		for (size_t meshIndex = 0; meshIndex < meshList.size(); ++meshIndex)
		{
			ObjLoader::MeshData& mesh = meshList[meshIndex];
			_verticesList.push_back(mesh.vertices);

			std::vector<std::vector<unsigned short>> subMeshIndices;
			std::vector<int> subMeshDiffuseTextureIndices;
			for (const auto& subMesh : mesh.subMeshMap)
			{
				subMeshIndices.push_back(subMesh.second);
				subMeshDiffuseTextureIndices.push_back(subMesh.first);
			}

			_indicesList.push_back(subMeshIndices);
			_diffuseTextureIndices.push_back(subMeshDiffuseTextureIndices);;
		}

		const std::string& fullPath = FileUtility::getInstance()->getFullPathForFileName(filePath);
		std::string textureBasePath = fullPath.substr(0, fullPath.find_last_of("\\/") + 1);
		if (textureBasePath.empty())
		{
			textureBasePath = "";
		}

		for (const ObjLoader::MaterialData& material : materialList)
		{
			// TODO:とりあえずdiffuseTextureだけに対応
			if (!material.diffuseTextureName.empty())
			{
				addTexture(textureBasePath + material.diffuseTextureName);
			}
		}
	}
	else if (ext == ".c3t" || ext == ".c3b")
	{
		_isC3b = true; // TODO:このフラグ分けは非常にださい

		_meshDatas = new (std::nothrow)C3bLoader::MeshDatas();
		C3bLoader::MaterialDatas* materialDatas = new (std::nothrow)C3bLoader::MaterialDatas();
		_nodeDatas = new (std::nothrow)C3bLoader::NodeDatas();
		_animationDatas = new (std::nothrow)C3bLoader::AnimationDatas();
		std::string err;
		if (ext == ".c3t")
		{
			err = C3bLoader::loadC3t(filePath, *_meshDatas, *materialDatas, *_nodeDatas, *_animationDatas);
		}
		else
		{
			Logger::logAssert(ext == ".c3b", "");
			err = C3bLoader::loadC3b(filePath, *_meshDatas, *materialDatas, *_nodeDatas, *_animationDatas);
		}

		if (!err.empty())
		{
			Logger::log(err.c_str());
			return false;
		}

		Logger::logAssert(_meshDatas->meshDatas.size() == 1, "現状メッシュ複数には対応してない。");
		C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
		_indicesList.push_back(meshData->subMeshIndices);

		_perVertexByteSize = 0;

		for (C3bLoader::MeshVertexAttribute attrib : meshData->attributes)
		{
			_perVertexByteSize += attrib.attributeSizeBytes;
		}

		C3bLoader::MaterialData* materialData = materialDatas->materialDatas[0];
		const C3bLoader::TextureData& texture = materialData->textures[0];
		addTexture(texture.fileName);
		_ambient = materialData->ambient;
		_diffuse = materialData->diffuse;
		_specular = materialData->specular;
		//_emissive = materialData->emissive;
		//_opacity = materialData->opacity;
		_shininess = materialData->shininess;

		delete materialDatas;
	}
	else
	{
		Logger::logAssert(false, "対応してない拡張子%s", ext);
	}

#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11Device* direct3dDevice = Director::getRenderer().getDirect3dDevice();
	HRESULT result = E_FAIL;

	if (_isObj)
	{
		// MeshDataの数のループ
		size_t numMesh = _verticesList.size();
		for (size_t meshIndex = 0; meshIndex < numMesh; ++meshIndex)
		{
			const std::vector<Position3DNormalTextureCoordinates>& vertices = _verticesList[meshIndex];
			// 頂点バッファの定義
			D3D11_BUFFER_DESC vertexBufferDesc;
			vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			vertexBufferDesc.ByteWidth = sizeof(Position3DNormalTextureCoordinates) * vertices.size();
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertexBufferDesc.CPUAccessFlags = 0;
			vertexBufferDesc.MiscFlags = 0;
			vertexBufferDesc.StructureByteStride = 0;

			// 頂点バッファのサブリソースの定義
			D3D11_SUBRESOURCE_DATA vertexBufferSubData;
			vertexBufferSubData.pSysMem = vertices.data();
			vertexBufferSubData.SysMemPitch = 0;
			vertexBufferSubData.SysMemSlicePitch = 0;

			// 頂点バッファのサブリソースの作成
			ID3D11Buffer* vertexBuffer = nullptr;
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

			std::vector<ID3D11Buffer*> indexBufferList;
			size_t numSubMesh = _indicesList[meshIndex].size();

			// subMeshの数のループ
			for (size_t subMeshIndex = 0; subMeshIndex < numSubMesh; ++subMeshIndex)
			{
				const std::vector<unsigned short>& subMeshIndices = _indicesList[meshIndex][subMeshIndex];

				// インデックスバッファの定義
				D3D11_BUFFER_DESC indexBufferDesc;
				indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
				indexBufferDesc.ByteWidth = sizeof(USHORT) * subMeshIndices.size();
				indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				indexBufferDesc.CPUAccessFlags = 0;
				indexBufferDesc.MiscFlags = 0;
				indexBufferDesc.StructureByteStride = 0;

				// インデックスバッファのサブリソースの定義
				D3D11_SUBRESOURCE_DATA indexBufferSubData;
				indexBufferSubData.pSysMem = subMeshIndices.data();
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

				indexBufferList.push_back(indexBuffer);
			}

			_d3dProgramForForwardRendering.addIndexBuffers(indexBufferList);
			_d3dProgramForShadowMap.addIndexBuffers(indexBufferList);
			_d3dProgramForPointLightShadowMap.addIndexBuffers(indexBufferList);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
			_d3dProgramForGBuffer.addIndexBuffers(indexBufferList);
#endif
		}

		bool depthEnable = true;
		_d3dProgramForForwardRendering.initWithShaderFile("Resources/shader/ObjForward.hlsl", depthEnable, "VS", "", "PS");
		_d3dProgramForShadowMap.initWithShaderFile("Resources/shader/Obj.hlsl", depthEnable, "VS_SM", "", "");
		_d3dProgramForPointLightShadowMap.initWithShaderFile("Resources/shader/Obj.hlsl", depthEnable, "VS_SM_POINT_LIGHT", "GS_SM_POINT_LIGHT", "");
#if defined(MGRRENDERER_DEFERRED_RENDERING)
		_d3dProgramForGBuffer.initWithShaderFile("Resources/shader/Obj.hlsl", depthEnable, "VS_GBUFFER", "", "PS_GBUFFER");
#endif

		// 入力レイアウトオブジェクトの作成
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{D3DProgram::SEMANTIC_POSITION.c_str(), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{D3DProgram::SEMANTIC_NORMAL.c_str(), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(Vec3), D3D11_INPUT_PER_VERTEX_DATA, 0},
			{D3DProgram::SEMANTIC_TEXTURE_COORDINATE.c_str(), 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(Vec3) * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
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
	}
	else if (_isC3b)
	{
		C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];

		// 頂点バッファの定義
		D3D11_BUFFER_DESC vertexBufferDesc;
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(float) * meshData->vertices.size();
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		// 頂点バッファのサブリソースの定義
		D3D11_SUBRESOURCE_DATA vertexBufferSubData;
		vertexBufferSubData.pSysMem = &meshData->vertices[0];
		vertexBufferSubData.SysMemPitch = 0;
		vertexBufferSubData.SysMemSlicePitch = 0;

		// 頂点バッファのサブリソースの作成
		ID3D11Buffer* vertexBuffer = nullptr;
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

		// インデックスバッファの定義
		D3D11_BUFFER_DESC indexBufferDesc;
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(USHORT) * _indicesList[0][0].size();
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		// インデックスバッファのサブリソースの定義
		D3D11_SUBRESOURCE_DATA indexBufferSubData;
		indexBufferSubData.pSysMem = _indicesList[0][0].data();
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
		_d3dProgramForShadowMap.addIndexBuffers(indexBufferList);
		_d3dProgramForPointLightShadowMap.addIndexBuffers(indexBufferList);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
		_d3dProgramForGBuffer.addIndexBuffers(indexBufferList);
#endif

		bool depthEnable = true;
		_d3dProgramForForwardRendering.initWithShaderFile("Resources/shader/C3bC3tForward.hlsl", depthEnable, "VS", "", "PS");
		_d3dProgramForShadowMap.initWithShaderFile("Resources/shader/C3bC3t.hlsl", depthEnable, "VS_SM", "", "");
		_d3dProgramForPointLightShadowMap.initWithShaderFile("Resources/shader/C3bC3t.hlsl", depthEnable, "VS_SM_POINT_LIGHT", "GS_SM_POINT_LIGHT", "");
#if defined(MGRRENDERER_DEFERRED_RENDERING)
		_d3dProgramForGBuffer.initWithShaderFile("Resources/shader/C3bC3t.hlsl", depthEnable, "VS_GBUFFER", "", "PS_GBUFFER");
#endif

		// 入力レイアウトオブジェクトの作成
		std::vector<D3D11_INPUT_ELEMENT_DESC> layouts(meshData->numAttribute);
		for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
		{
			const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
			D3D11_INPUT_ELEMENT_DESC layout = {attrib.semantic.c_str(), 0, D3DProgram::getDxgiFormat(attrib.semantic), 0, static_cast<UINT>(offset), D3D11_INPUT_PER_VERTEX_DATA, 0};
			layouts[i] = layout;
			offset += attrib.attributeSizeBytes;
		}

		ID3D11InputLayout* inputLayout = nullptr;
		result = direct3dDevice->CreateInputLayout(
			&layouts[0],
			layouts.size(), 
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
	}

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
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);
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
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX, constantBuffer);
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
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer);
#endif

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
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える
#endif

	// スキニングのマトリックスパレット
	if (_isC3b)
	{
		constantBufferDesc.ByteWidth = sizeof(Mat4) * MAX_SKINNING_JOINT;
		constantBuffer = nullptr;
		result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
		if (FAILED(result))
		{
			Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
			return false;
		}
		_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE, constantBuffer);
		_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える
		_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える
#if defined(MGRRENDERER_DEFERRED_RENDERING)
		_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE, constantBuffer);
#endif
	}

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
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える

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
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer);
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える

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
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer); // シェーダでは使わないが、インデックスの数値を共有しているのでずれないようにシャドウマップ用定数バッファにも加える
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

	constantBufferDesc.ByteWidth = sizeof(SpotLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER, constantBuffer);

#elif defined(MGRRENDERER_USE_OPENGL)
	if (_isObj)
	{
		_glProgramForForwardRendering.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderPositionNormalTexture3D.glsl", "../MGRRenderer/Resources/shader/FragmentShaderPositionTextureNormalMultiplyColor3D.glsl");
	}
	else if (_isC3b)
	{
		_glProgramForForwardRendering.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderC3bC3t.glsl", "../MGRRenderer/Resources/shader/FragmentShaderC3bC3t.glsl");
	}

	// TODO:ライトの判定入れないと

	if (_isObj)
	{
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
	}
	else if (_isC3b)
	{
		_glProgramForShadowMap.initWithShaderString(
			// vertex shader
			// ModelDataしか使わない場合
			//"attribute vec4 a_position;"
			//"uniform mat4 u_lightViewMatrix;" // 影付けに使うライトをカメラに見立てたビュー行列
			//"uniform mat4 u_viewMatrix;"
			//"uniform mat4 u_projectionMatrix;"
			//"void main()"
			//"{"
			//"	gl_Position = u_projectionMatrix * u_lightViewMatrix * u_modelMatrix * getPosition();"
			//"}"
			//// アニメーションを使う場合
			"#version 430\n"
			"attribute vec3 a_position;" // これがvec3になっているのに注意 TODO:なぜなのか？
			"attribute vec4 a_blendWeight;"
			"attribute vec4 a_blendIndex;"
			""
			"const int MAX_SKINNING_JOINT = 60;" // TODO:なぜ60個までなのか？
			""
			"uniform mat4 u_modelMatrix;"
			"uniform mat4 u_lightViewMatrix;" // 影付けに使うライトをカメラに見立てたビュー行列
			"uniform mat4 u_lightProjectionMatrix;"
			"uniform mat4 u_matrixPalette[MAX_SKINNING_JOINT];"
			""
			"varying vec2 v_texCoord;"
			""
			"vec4 getPosition()"
			"{"
			"	mat4 skinMatrix = u_matrixPalette[int(a_blendIndex[0])] * a_blendWeight[0];"
			""
			"	if (a_blendWeight[1] > 0.0)"
			"	{"
			"		skinMatrix += u_matrixPalette[int(a_blendIndex[1])] * a_blendWeight[1];"
			""
			"		if (a_blendWeight[2] > 0.0)"
			"		{"
			"			skinMatrix += u_matrixPalette[int(a_blendIndex[2])] * a_blendWeight[2];"
			""
			"			if (a_blendWeight[3] > 0.0)"
			"			{"
			"				skinMatrix += u_matrixPalette[int(a_blendIndex[3])] * a_blendWeight[3];"
			"			}"
			"		}"
			"	}"
			""
			"	vec4 position = vec4(a_position, 1.0);"
			"	vec4 skinnedPosition = skinMatrix * position;"
			"	skinnedPosition.w = 1.0;"
			"	return skinnedPosition;"
			"}"
			""
			"void main()"
			"{"
			"	gl_Position = u_lightProjectionMatrix * u_lightViewMatrix * u_modelMatrix * getPosition();"
			"}"
			,
			// fragment shader
			"#version 430\n"
			"void main()"
			"{" // 何もせずとも深度は自動で書き込まれる 
			"}"
		);
	}

	if (_isObj)
	{
		// STRINGIFYによる読み込みだと、GeForce850Mがうまく#versionの行の改行を読み取ってくれずGLSLコンパイルエラーになる
		_glProgramForGBuffer.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderObj.glsl", "../MGRRenderer/Resources/shader/FragmentShaderPositionNormalTextureMultiplyColorGBuffer.glsl");
	}
	else if (_isC3b)
	{
		_glProgramForGBuffer.initWithShaderString(
			// vertex shader
			// ModelDataしか使わない場合
			//"attribute vec4 a_position;"
			//"attribute vec2 a_texCoord;"
			//"varying vec2 v_texCoord;"
			//"uniform mat4 u_modelMatrix;"
			//"uniform mat4 u_viewMatrix;"
			//"uniform mat4 u_projectionMatrix;"
			//"void main()"
			//"{"
			//"	gl_Position = u_projectionMatrix * u_viewMatrix * a_position;"
			//"	v_texCoord = a_texCoord;"
			//"	v_texCoord.y = 1.0 - v_texCoord.y;"
			//"}"
			//// アニメーションを使う場合
			"#version 430\n"
			""
			"in vec3 a_position;" // これがvec3になっているのに注意 TODO:なぜなのか？
			"in vec3 a_normal;"
			"in vec2 a_texCoord;"
			"in vec4 a_blendWeight;"
			"in vec4 a_blendIndex;"
			""
			"const int MAX_SKINNING_JOINT = 60;" // TODO:なぜ60個までなのか？
			""
			"uniform mat4 u_modelMatrix;"
			"uniform mat4 u_viewMatrix;"
			"uniform mat4 u_projectionMatrix;"
			"uniform mat4 u_normalMatrix;" // scale変換に対応するためにモデル行列の逆行列を転置したものを用いる
			"uniform mat4 u_matrixPalette[MAX_SKINNING_JOINT];"
			""
			"out vec4 v_normal;"
			"out vec2 v_texCoord;"
			""
			"vec4 getPosition()"
			"{"
			"	mat4 skinMatrix = u_matrixPalette[int(a_blendIndex[0])] * a_blendWeight[0];"
			""
			"	if (a_blendWeight[1] > 0.0)"
			"	{"
			"		skinMatrix += u_matrixPalette[int(a_blendIndex[1])] * a_blendWeight[1];"
			""
			"		if (a_blendWeight[2] > 0.0)"
			"		{"
			"			skinMatrix += u_matrixPalette[int(a_blendIndex[2])] * a_blendWeight[2];"
			""
			"			if (a_blendWeight[3] > 0.0)"
			"			{"
			"				skinMatrix += u_matrixPalette[int(a_blendIndex[3])] * a_blendWeight[3];"
			"			}"
			"		}"
			"	}"
			""
			"	vec4 position = vec4(a_position, 1.0);"
			"	vec4 skinnedPosition = skinMatrix * position;"
			"	skinnedPosition.w = 1.0;"
			"	return skinnedPosition;"
			"}"
			""
			"void main()"
			"{"
			"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * getPosition();"
			"	vec4 normal = vec4(a_normal, 1.0);"
			"	v_normal = vec4(normalize((u_normalMatrix * normal).xyz), 1.0);"
			"	v_texCoord = a_texCoord;"
			"	v_texCoord.y = 1.0 - v_texCoord.y;" // c3bの事情によるもの
			"}"
			,
			// fragment shader
			"#version 430\n"
			""
			"float SPECULAR_POWER_RANGE_X = 10.0;"
			"float SPECULAR_POWER_RANGE_Y = 250.0;"
			""
			"in vec4 v_position;"
			"in vec4 v_normal;"
			"in vec2 v_texCoord;"
			""
			"uniform vec3 u_multipleColor;"
			"uniform sampler2D u_texture;"
			""
			"layout (location = 0) out vec4 FragColor;" // デプスバッファの分
			"layout (location = 1) out vec4 ColorSpecularIntensity;"
			"layout (location = 2) out vec4 Normal;"
			"layout (location = 3) out vec4 SpecularPower;"
			""
			"void main()"
			"{"
			"	"// Gバッファへのパッキングを行う
			""
			"	"// specularは今のところ対応してない
			"	float specularPower = 0.0;"
			"	float specularIntensity = 0.0;"
			"	float specularPowerNorm = max(0.0001, (specularPower - SPECULAR_POWER_RANGE_X) / SPECULAR_POWER_RANGE_Y);"
			""
			"	ColorSpecularIntensity = vec4(texture2D(u_texture, v_texCoord).rgb * u_multipleColor.rgb, specularIntensity);"
			"	Normal = vec4(v_normal.xyz * 0.5 + 0.5, 0.0);"
			"	SpecularPower = vec4(specularPowerNorm, 0.0, 0.0, 0.0);"
			"}"
		);
	}

#endif

	return true;
}

void Sprite3D::addTexture(const std::string& filePath)
{
	Image image; // ImageはCPU側のメモリを使っているのでこのスコープで解放されてもよいものだからスタックに取る
	bool success = image.initWithFilePath(filePath);
	Logger::logAssert(success, "Sprite3Dでテクスチャ作成に失敗。");
	if (!success)
	{
		return;
	}

	// TextureはGPU側のメモリを使ってるので解放されると困るのでヒープにとる
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DTexture* texture = new D3DTexture(); 
#elif defined(MGRRENDERER_USE_OPENGL)
	GLTexture* texture = new GLTexture();
#endif
	success = static_cast<Texture*>(texture)->initWithImage(image); // TODO:なぜか暗黙に継承元クラスのメソッドが呼べない
	Logger::logAssert(success, "Sprite3Dでテクスチャ作成に失敗。");
	if (success)
	{
		_textureList.push_back(texture);
	}
}

void Sprite3D::startAnimation(const std::string& animationName, bool loop /* = false*/)
{
	_elapsedTime = 0.0f;
	_currentAnimation = _animationDatas->animations[animationName];
	_loopAnimation = loop;
}

void Sprite3D::stopAnimation()
{
	_currentAnimation = nullptr;
}

C3bLoader::NodeData* Sprite3D::findJointByName(const std::string& jointName, const std::vector<C3bLoader::NodeData*> children)
{
	for (C3bLoader::NodeData* child : children)
	{
		if (child->id == jointName)
		{
			// 見つかったらそれを返す。ループも終了。
			return child;
		}
		else if (child->children.size() > 0)
		{
			// 見つからず、子がいるなら子を探しに行く
			C3bLoader::NodeData* findResult = findJointByName(jointName, child->children);
			if (findResult != nullptr)
			{
				// 見つかったらそれを返す。ループも終了。
				return findResult;
			}
		}
	}

	return nullptr;
}

void Sprite3D::update(float dt)
{
	Node::update(dt);

	if (!_isC3b) {
		return;
	}

	float t = 0; // 0<=t<=1のアニメーション補完パラメータ
	if (_currentAnimation != nullptr)
	{
		_elapsedTime += dt;
		if (_loopAnimation && _elapsedTime > _currentAnimation->totalTime)
		{
			_elapsedTime = 0.0f;
		}

		t = _elapsedTime / _currentAnimation->totalTime;
	}

	_matrixPalette.clear();

	// アニメーションを行う。Animate3D::updateを参考に C3bLoader::AnimationDataの使いこなしが肝だ
	size_t numSkinJoint = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones.size();
	Logger::logAssert(numSkinJoint == _nodeDatas->nodes[0]->modelNodeDatas[0]->invBindPose.size(), "ジョイント数は一致するはず");

	// 先に各ジョイントのアニメーション行列を作成する
	for (size_t i = 0; i < numSkinJoint; ++i)
	{
		const std::string& jointName = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones[i];//TODO: nodes下に要素は一個、parts下にも一個だけであることを前提にしている

		// キーフレーム情報がなかったら、NodeDatas::skeletons::transformの方のツリーから再帰的にジョイント位置座標変換配列を計算する
		// まずは対応するボーンを探し、設定されている行列を取得する
		C3bLoader::NodeData* node = findJointByName(jointName, _nodeDatas->skeleton);
		Logger::logAssert(node != nullptr, "nodesの方にあったものでskeletonsからボーン名で検索したがない。");

		// 一旦、アニメーション使わない方に初期化
		node->animatedTransform = Mat4::ZERO;

		if (_currentAnimation != nullptr && _currentAnimation->translationKeyFrames.find(jointName) != _currentAnimation->translationKeyFrames.end())
		{
			// アニメーション中で、データにそのボーンのキーフレーム情報があったときはそちらから作った行列で上書きする
			const Vec3& translation = _currentAnimation->evaluateTranslation(jointName, t);
			const Quaternion& rotation = _currentAnimation->evaluateRotation(jointName, t);
			const Vec3& scale = _currentAnimation->evaluateScale(jointName, t);

			node->animatedTransform = Mat4::createTransform(translation, rotation, scale);
		}
	}

	// 次にジョイントのマトリックスパレットをすべて求める
	for (size_t i = 0; i < numSkinJoint; ++i)
	{
		const std::string& jointName = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones[i];//TODO: nodes下に要素は一個、parts下にも一個だけであることを前提にしている

		// キーフレーム情報がなかったら、NodeDatas::skeletons::transformの方のツリーから再帰的にジョイント位置座標変換配列を計算する
		// まずは対応するボーンを探し、設定されている行列を取得する
		C3bLoader::NodeData* node = findJointByName(jointName, _nodeDatas->skeleton);
		if (node == nullptr)
		{
			Logger::log("nodesの方にあったものでskeletonsからボーン名で検索したがない。");
		}
		Logger::logAssert(node != nullptr, "c3bの仕様上、ジョイント名が見つからないはずがない。");

		Mat4 transform = (node->animatedTransform != Mat4::ZERO) ? node->animatedTransform : node->transform;

		// 親をルートまでさかのぼってワールド行列を計算する
		while (node->parent != nullptr)
		{
			// TODO:親のボーンもアニメーションしてる可能性あったために、NodeDataにもう一個Matrix持たせるという強引な方法。本来はSkeleton3DとBone3Dみたいに別の階層構造データを保持した方がいい
			const Mat4& parentTransform = (node->parent->animatedTransform != Mat4::ZERO) ? node->parent->animatedTransform : node->parent->transform;
			transform = parentTransform * transform;
			node = node->parent;
		}

		// ボーンの配置行列
		const Mat4& invBindPose = _nodeDatas->nodes[0]->modelNodeDatas[0]->invBindPose[i];//TODO: nodes下に要素は一個、parts下にも一個だけであることを前提にしている

		Mat4 matrix = transform * invBindPose;
#if defined(MGRRENDERER_USE_DIRECT3D)
		matrix.transpose();
#endif
		_matrixPalette.push_back(matrix);
	}
}

#if defined(MGRRENDERER_DEFERRED_RENDERING)
void Sprite3D::renderGBuffer()
{
	_renderGBufferCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();

		// TODO:ここらへん共通化したいな。。
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

		if (_isC3b)
		{
			// ジョイントマトリックスパレットのマップ
			result = direct3dContext->Map(
				_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
			CopyMemory(mappedResource.pData, _matrixPalette.data(), sizeof(Mat4) * _matrixPalette.size());
			direct3dContext->Unmap(_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE), 0);
		}

		size_t stride = 0;
		size_t offset = 0;
		if (_isObj)
		{
			stride = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			stride = _perVertexByteSize;
		}

		size_t numVertexBuffer = _d3dProgramForGBuffer.getVertexBuffers().size();
		std::vector<UINT> strides(numVertexBuffer, stride);
		std::vector<UINT> offsets(numVertexBuffer, offset);

		direct3dContext->IASetVertexBuffers(0, _d3dProgramForGBuffer.getVertexBuffers().size(), _d3dProgramForGBuffer.getVertexBuffers().data(), strides.data(), offsets.data());
		direct3dContext->IASetIndexBuffer(_d3dProgramForGBuffer.getIndexBuffers()[0][0], DXGI_FORMAT_R16_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForGBuffer.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForGBuffer.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForGBuffer.setConstantBuffersToDirect3DContext(direct3dContext);

		ID3D11ShaderResourceView* resourceView[1] = { _textureList[0]->getShaderResourceView() };
		direct3dContext->PSSetShaderResources(0, 1, resourceView);
		ID3D11SamplerState* samplerState[1] = { Director::getRenderer().getLinearSamplerState() };
		direct3dContext->PSSetSamplers(0, 1, samplerState);

		direct3dContext->DrawIndexed(_indicesList[0][0].size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForGBuffer.getShaderProgram());
		GLProgram::checkGLError();

		glUniform3f(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		GLProgram::checkGLError();

		// 行列の設定
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		GLProgram::checkGLError();

		Mat4 normalMatrix = Mat4::createNormalMatrix(getModelMatrix());
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_NORMAL_MATRIX), 1, GL_FALSE, (GLfloat*)&normalMatrix.m);

		// 頂点属性の設定
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_WEIGHT);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_INDEX);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
		GLProgram::checkGLError();

		if (_isObj)
		{
			// メッシュ数のループ
			for (size_t meshIndex = 0; meshIndex < _verticesList.size(); ++meshIndex)
			{
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].position);
				GLProgram::checkGLError();
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].normal);
				GLProgram::checkGLError();
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].textureCoordinate);
				GLProgram::checkGLError();

				glActiveTexture(GL_TEXTURE0);

				GLuint textureId = _textureList[0]->getTextureId();
				size_t numSubMesh = _indicesList[meshIndex].size();
				for (size_t subMeshIndex = 0; subMeshIndex < numSubMesh; ++subMeshIndex)
				{
					int subMeshDiffuseTextureIndex = _diffuseTextureIndices[meshIndex][subMeshIndex];
					if (_useMtl)
					{
						textureId = _textureList[subMeshDiffuseTextureIndex]->getTextureId();
					}

					glBindTexture(GL_TEXTURE_2D, textureId);
					GLProgram::checkGLError();

					const std::vector<unsigned short>& subMeshIndices = _indicesList[meshIndex][subMeshIndex];
					glDrawElements(GL_TRIANGLES, subMeshIndices.size(), GL_UNSIGNED_SHORT, subMeshIndices.data());
					GLProgram::checkGLError();
				}

				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
		else if (_isC3b)
		{
			// TODO:objあるいはc3t/c3bでメッシュデータは一個である前提
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				GLProgram::checkGLError();
				offset += attrib.size;
			}

			Logger::logAssert(_matrixPalette.size() > 0, "マトリックスパレットは0でない前提");
			glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette.data()));
			GLProgram::checkGLError();

			glBindTexture(GL_TEXTURE_2D, _textureList[0]->getTextureId());
			GLProgram::checkGLError();

			glDrawElements(GL_TRIANGLES, _indicesList[0].size(), GL_UNSIGNED_SHORT, _indicesList[0].data());
			GLProgram::checkGLError();
			glBindTexture(GL_TEXTURE_2D, 0);
		}
#endif
	});

	Director::getRenderer().addCommand(&_renderGBufferCommand);
}
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

void Sprite3D::renderDirectionalLightShadowMap(const DirectionalLight* light)
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

		if (_isC3b)
		{
			// ジョイントマトリックスパレットのマップ
			result = direct3dContext->Map(
				_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
			CopyMemory(mappedResource.pData, _matrixPalette.data(), sizeof(Mat4) * _matrixPalette.size());
			direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE), 0);
		}

		size_t stride = 0;
		size_t offset = 0;
		if (_isObj)
		{
			stride = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			stride = _perVertexByteSize;
		}

		size_t numVertexBuffer = _d3dProgramForShadowMap.getVertexBuffers().size();
		std::vector<UINT> strides(numVertexBuffer, stride);
		std::vector<UINT> offsets(numVertexBuffer, offset);

		direct3dContext->IASetVertexBuffers(0, _d3dProgramForShadowMap.getVertexBuffers().size(), _d3dProgramForShadowMap.getVertexBuffers().data(), strides.data(), offsets.data());
		direct3dContext->IASetInputLayout(_d3dProgramForShadowMap.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForShadowMap.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForShadowMap.setConstantBuffersToDirect3DContext(direct3dContext);

		if (_isObj)
		{
			// メッシュ数のループ
			for (size_t meshIndex = 0; meshIndex < _verticesList.size(); ++meshIndex)
			{
				size_t numSubMesh = _indicesList[meshIndex].size();
				for (size_t subMeshIndex = 0; subMeshIndex < numSubMesh; ++subMeshIndex)
				{
					direct3dContext->IASetIndexBuffer(_d3dProgramForShadowMap.getIndexBuffers()[meshIndex][subMeshIndex], DXGI_FORMAT_R16_UINT, 0);
					direct3dContext->DrawIndexed(_indicesList[meshIndex][subMeshIndex].size(), 0, 0);
				}
			}
		}
		else if (_isC3b)
		{
			direct3dContext->IASetIndexBuffer(_d3dProgramForShadowMap.getIndexBuffers()[0][0], DXGI_FORMAT_R16_UINT, 0);
			direct3dContext->DrawIndexed(_indicesList[0][0].size(), 0, 0);
		}
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		GLProgram::checkGLError();

		// 行列の設定
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

		// 頂点属性の設定
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_WEIGHT);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_INDEX);
		GLProgram::checkGLError();

		if (_isObj)
		{
			// メッシュ数のループ
			for (size_t meshIndex = 0; meshIndex < _verticesList.size(); ++meshIndex)
			{
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].position);
				GLProgram::checkGLError();
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].normal);
				GLProgram::checkGLError();
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].textureCoordinate);
				GLProgram::checkGLError();

				size_t numSubMesh = _indicesList[meshIndex].size();
				for (size_t subMeshIndex = 0; subMeshIndex < numSubMesh; ++subMeshIndex)
				{
					const std::vector<unsigned short>& subMeshIndices = _indicesList[meshIndex][subMeshIndex];
					glDrawElements(GL_TRIANGLES, subMeshIndices.size(), GL_UNSIGNED_SHORT, subMeshIndices.data());
					GLProgram::checkGLError();
				}
			}
		}
		else if (_isC3b)
		{
			// TODO:c3t/c3bでメッシュデータは一個である前提
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				GLProgram::checkGLError();
				offset += attrib.size;
			}

			// スキニングのマトリックスパレットの設定
			Logger::logAssert(_matrixPalette.size() > 0, "マトリックスパレットは0でない前提");
			glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette[0].m));
			GLProgram::checkGLError();

			glDrawElements(GL_TRIANGLES, _indicesList[0].size(), GL_UNSIGNED_SHORT, _indicesList[0].data());
			GLProgram::checkGLError();
		}
#endif
	});

	Director::getRenderer().addCommand(&_renderDirectionalLightShadowMapCommand);
}

void Sprite3D::renderPointLightShadowMap(size_t index, const PointLight* light, CubeMapFace face)
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

		if (_isC3b)
		{
			// ジョイントマトリックスパレットのマップ
			result = direct3dContext->Map(
				_d3dProgramForPointLightShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
			CopyMemory(mappedResource.pData, _matrixPalette.data(), sizeof(Mat4) * _matrixPalette.size());
			direct3dContext->Unmap(_d3dProgramForPointLightShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE), 0);
		}

		size_t stride = 0;
		size_t offset = 0;
		if (_isObj)
		{
			stride = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			stride = _perVertexByteSize;
		}

		size_t numVertexBuffer = _d3dProgramForPointLightShadowMap.getVertexBuffers().size();
		std::vector<UINT> strides(numVertexBuffer, stride);
		std::vector<UINT> offsets(numVertexBuffer, offset);

		direct3dContext->IASetVertexBuffers(0, numVertexBuffer, _d3dProgramForPointLightShadowMap.getVertexBuffers().data(), strides.data(), offsets.data());
		direct3dContext->IASetInputLayout(_d3dProgramForPointLightShadowMap.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForPointLightShadowMap.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForPointLightShadowMap.setConstantBuffersToDirect3DContext(direct3dContext);

		if (_isObj)
		{
			// メッシュ数のループ
			for (size_t meshIndex = 0; meshIndex < _verticesList.size(); ++meshIndex)
			{
				size_t numSubMesh = _indicesList[meshIndex].size();
				for (size_t subMeshIndex = 0; subMeshIndex < numSubMesh; ++subMeshIndex)
				{
					direct3dContext->IASetIndexBuffer(_d3dProgramForPointLightShadowMap.getIndexBuffers()[meshIndex][subMeshIndex], DXGI_FORMAT_R16_UINT, 0);
					direct3dContext->DrawIndexed(_indicesList[meshIndex][subMeshIndex].size(), 0, 0);
				}
			}
		}
		else if (_isC3b)
		{
			direct3dContext->IASetIndexBuffer(_d3dProgramForPointLightShadowMap.getIndexBuffers()[0][0], DXGI_FORMAT_R16_UINT, 0);
			direct3dContext->DrawIndexed(_indicesList[0][0].size(), 0, 0);
		}
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		GLProgram::checkGLError();

		// 行列の設定
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

		// 頂点属性の設定
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_WEIGHT);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_INDEX);
		GLProgram::checkGLError();

		// TODO:objあるいはc3t/c3bでメッシュデータは一個である前提
		if (_isObj)
		{
			// メッシュ数のループ
			for (size_t meshIndex = 0; meshIndex < _verticesList.size(); ++meshIndex)
			{
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].position);
				GLProgram::checkGLError();
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].normal);
				GLProgram::checkGLError();
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].textureCoordinate);
				GLProgram::checkGLError();

				size_t numSubMesh = _indicesList[meshIndex].size();
				for (size_t subMeshIndex = 0; subMeshIndex < numSubMesh; ++subMeshIndex)
				{
					const std::vector<unsigned short>& subMeshIndices = _indicesList[meshIndex][subMeshIndex];
					glDrawElements(GL_TRIANGLES, subMeshIndices.size(), GL_UNSIGNED_SHORT, subMeshIndices.data());
					GLProgram::checkGLError();
				}
			}
		}
		else if (_isC3b)
		{
			// TODO:objあるいはc3t/c3bでメッシュデータは一個である前提
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				GLProgram::checkGLError();
				offset += attrib.size;
			}

			// スキニングのマトリックスパレットの設定
			Logger::logAssert(_matrixPalette.size() > 0, "マトリックスパレットは0でない前提");
			glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette[0].m));
			GLProgram::checkGLError();

			glDrawElements(GL_TRIANGLES, _indicesList[0].size(), GL_UNSIGNED_SHORT, _indicesList[0].data());
			GLProgram::checkGLError();
		}
#endif
	});

	Director::getRenderer().addCommand(&_renderPointLightShadowMapCommandList[index][(size_t)face]);
}

void Sprite3D::renderSpotLightShadowMap(size_t index, const SpotLight* light)
{
	Logger::logAssert(light != nullptr, "ライト引数はnullでない前提。");
	Logger::logAssert(light->hasShadowMap(), "ライトはシャドウを持っている前提。");

	_renderSpotLightShadowMapCommandList[index].init([=]
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

		if (_isC3b)
		{
			// ジョイントマトリックスパレットのマップ
			result = direct3dContext->Map(
				_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
			CopyMemory(mappedResource.pData, _matrixPalette.data(), sizeof(Mat4) * _matrixPalette.size());
			direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE), 0);
		}

		size_t stride = 0;
		size_t offset = 0;
		if (_isObj)
		{
			stride = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			stride = _perVertexByteSize;
		}

		size_t numVertexBuffer = _d3dProgramForShadowMap.getVertexBuffers().size();
		std::vector<UINT> strides(numVertexBuffer, stride);
		std::vector<UINT> offsets(numVertexBuffer, offset);

		direct3dContext->IASetVertexBuffers(0, _d3dProgramForShadowMap.getVertexBuffers().size(), _d3dProgramForShadowMap.getVertexBuffers().data(), strides.data(), offsets.data());
		direct3dContext->IASetInputLayout(_d3dProgramForShadowMap.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForShadowMap.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForShadowMap.setConstantBuffersToDirect3DContext(direct3dContext);

		if (_isObj)
		{
			// メッシュ数のループ
			for (size_t meshIndex = 0; meshIndex < _verticesList.size(); ++meshIndex)
			{
				size_t numSubMesh = _indicesList[meshIndex].size();
				for (size_t subMeshIndex = 0; subMeshIndex < numSubMesh; ++subMeshIndex)
				{
					direct3dContext->IASetIndexBuffer(_d3dProgramForShadowMap.getIndexBuffers()[meshIndex][subMeshIndex], DXGI_FORMAT_R16_UINT, 0);
					direct3dContext->DrawIndexed(_indicesList[meshIndex][subMeshIndex].size(), 0, 0);
				}
			}
		}
		else if (_isC3b)
		{
			direct3dContext->IASetIndexBuffer(_d3dProgramForShadowMap.getIndexBuffers()[0][0], DXGI_FORMAT_R16_UINT, 0);
			direct3dContext->DrawIndexed(_indicesList[0][0].size(), 0, 0);
		}
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		GLProgram::checkGLError();

		// 行列の設定
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

		// 頂点属性の設定
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_WEIGHT);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_INDEX);
		GLProgram::checkGLError();

		// TODO:objあるいはc3t/c3bでメッシュデータは一個である前提
		if (_isObj)
		{
			// メッシュ数のループ
			for (size_t meshIndex = 0; meshIndex < _verticesList.size(); ++meshIndex)
			{
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].position);
				GLProgram::checkGLError();
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].normal);
				GLProgram::checkGLError();
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].textureCoordinate);
				GLProgram::checkGLError();

				size_t numSubMesh = _indicesList[meshIndex].size();
				for (size_t subMeshIndex = 0; subMeshIndex < numSubMesh; ++subMeshIndex)
				{
					const std::vector<unsigned short>& subMeshIndices = _indicesList[meshIndex][subMeshIndex];
					glDrawElements(GL_TRIANGLES, subMeshIndices.size(), GL_UNSIGNED_SHORT, subMeshIndices.data());
					GLProgram::checkGLError();
				}
			}
		}
		else if (_isC3b)
		{
			// TODO:objあるいはc3t/c3bでメッシュデータは一個である前提
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				GLProgram::checkGLError();
				offset += attrib.size;
			}

			// スキニングのマトリックスパレットの設定
			Logger::logAssert(_matrixPalette.size() > 0, "マトリックスパレットは0でない前提");
			glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette[0].m));
			GLProgram::checkGLError();

			glDrawElements(GL_TRIANGLES, _indicesList[0].size(), GL_UNSIGNED_SHORT, _indicesList[0].data());
			GLProgram::checkGLError();
		}
#endif
	});

	Director::getRenderer().addCommand(&_renderSpotLightShadowMapCommandList[index]);
}

void Sprite3D::renderForward()
{
	_renderForwardCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();

		// TODO:ここらへん共通化したいな。。
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


		if (_isC3b)
		{
			// ジョイントマトリックスパレットのマップ
			result = direct3dContext->Map(
				_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
			CopyMemory(mappedResource.pData, _matrixPalette.data(), sizeof(Mat4) * _matrixPalette.size());
			direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE), 0);
		}


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


		size_t stride = 0;
		size_t offset = 0;
		if (_isObj)
		{
			stride = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			stride = _perVertexByteSize;
		}

		size_t numVertexBuffer = _d3dProgramForForwardRendering.getVertexBuffers().size();
		std::vector<UINT> strides(numVertexBuffer, stride);
		std::vector<UINT> offsets(numVertexBuffer, offset);

		direct3dContext->IASetVertexBuffers(0, _d3dProgramForForwardRendering.getVertexBuffers().size(), _d3dProgramForForwardRendering.getVertexBuffers().data(), strides.data(), offsets.data());
		direct3dContext->IASetInputLayout(_d3dProgramForForwardRendering.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForForwardRendering.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForForwardRendering.setConstantBuffersToDirect3DContext(direct3dContext);

		direct3dContext->PSSetShaderResources(2, pointLightShadowCubeMapResourceView.size(), pointLightShadowCubeMapResourceView.data());

		direct3dContext->PSSetShaderResources(2 + pointLightShadowCubeMapResourceView.size(), spotLightShadowMapResourceView.size(), spotLightShadowMapResourceView.data());

		ID3D11SamplerState* samplerState[2] = { Director::getRenderer().getLinearSamplerState(), Director::getRenderer().getPCFSamplerState() };
		direct3dContext->PSSetSamplers(0, 2, samplerState);

		if (_isObj)
		{
			// メッシュ数のループ
			for (size_t meshIndex = 0; meshIndex < _verticesList.size(); ++meshIndex)
			{
				D3DTexture* texture = _textureList[0];
				size_t numSubMesh = _indicesList[meshIndex].size();
				for (size_t subMeshIndex = 0; subMeshIndex < numSubMesh; ++subMeshIndex)
				{
					int subMeshDiffuseTextureIndex = _diffuseTextureIndices[meshIndex][subMeshIndex];
					if (_useMtl)
					{
						texture = _textureList[subMeshDiffuseTextureIndex];
					}

					ID3D11ShaderResourceView* shaderResourceViews[2] = {
						texture->getShaderResourceView(),
						dirLightShadowMapResourceView,
					};
					direct3dContext->PSSetShaderResources(0, 2, shaderResourceViews);

					direct3dContext->IASetIndexBuffer(_d3dProgramForForwardRendering.getIndexBuffers()[0][0], DXGI_FORMAT_R16_UINT, 0);
					direct3dContext->DrawIndexed(_indicesList[0][0].size(), 0, 0);
				}
			}
		}
		else if (_isC3b)
		{
			ID3D11ShaderResourceView* shaderResourceViews[2] = {
				_textureList[0]->getShaderResourceView(),
				dirLightShadowMapResourceView,
			};
			direct3dContext->PSSetShaderResources(0, 2, shaderResourceViews);

			direct3dContext->IASetIndexBuffer(_d3dProgramForForwardRendering.getIndexBuffers()[0][0], DXGI_FORMAT_R16_UINT, 0);
			direct3dContext->DrawIndexed(_indicesList[0][0].size(), 0, 0);
		}
#elif defined(MGRRENDERER_USE_OPENGL)
		// cocos2d-xはTriangleCommand発行してる形だからな。。テクスチャバインドはTexture2Dでやってるのに大丈夫か？
		glUseProgram(_glProgramForForwardRendering.getShaderProgram());
		GLProgram::checkGLError();

		glUniform1i(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_RENDER_MODE), (GLint)Director::getRenderer().getRenderMode());

		glUniform4f(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f, getOpacity());
		GLProgram::checkGLError();

		// 行列の設定
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

				glActiveTexture(GL_TEXTURE1);
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

					glActiveTexture(GL_TEXTURE2 + i);
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

					glActiveTexture(GL_TEXTURE6 + i);
					GLuint textureId = spotLight->getShadowMapData().getDepthTexture()->getTextureId();
					glBindTexture(GL_TEXTURE_2D, textureId);
					glUniform1i(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_spotLightShadowMap[") + std::to_string(i) + std::string("]")).c_str()), 5 + i);
					//glUniform1i(_glProgramForForwardRendering.getUniformLocation(std::string("u_spotLightShadowMap[") + std::to_string(i) + std::string("]")), 9 + i);
					glActiveTexture(GL_TEXTURE0);
				}
			}
		}

		// 頂点属性の設定
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_WEIGHT);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_INDEX);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
		GLProgram::checkGLError();

		if (_isObj)
		{
			// メッシュ数のループ
			for (size_t meshIndex = 0; meshIndex < _verticesList.size(); ++meshIndex)
			{
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].position);
				GLProgram::checkGLError();
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].normal);
				GLProgram::checkGLError();
				glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_verticesList[meshIndex][0].textureCoordinate);
				GLProgram::checkGLError();

				glActiveTexture(GL_TEXTURE0);

				GLuint textureId = _textureList[0]->getTextureId();
				size_t numSubMesh = _indicesList[meshIndex].size();
				for (size_t subMeshIndex = 0; subMeshIndex < numSubMesh; ++subMeshIndex)
				{
					int subMeshDiffuseTextureIndex = _diffuseTextureIndices[meshIndex][subMeshIndex];
					if (_useMtl)
					{
						textureId = _textureList[subMeshDiffuseTextureIndex]->getTextureId();
					}

					glBindTexture(GL_TEXTURE_2D, textureId);
					GLProgram::checkGLError();

					const std::vector<unsigned short>& subMeshIndices = _indicesList[meshIndex][subMeshIndex];
					glDrawElements(GL_TRIANGLES, subMeshIndices.size(), GL_UNSIGNED_SHORT, subMeshIndices.data());
					GLProgram::checkGLError();
				}

				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
		else if (_isC3b)
		{
			// TODO:objあるいはc3t/c3bでメッシュデータは一個である前提
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				GLProgram::checkGLError();
				offset += attrib.size;
			}

			// スキニングのマトリックスパレットの設定
			Logger::logAssert(_matrixPalette.size() > 0, "マトリックスパレットは0でない前提");
			glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette.data()));
			GLProgram::checkGLError();

			glBindTexture(GL_TEXTURE_2D, _textureList[0]->getTextureId());
			GLProgram::checkGLError();

			glDrawElements(GL_TRIANGLES, _indicesList[0].size(), GL_UNSIGNED_SHORT, _indicesList[0].data());
			GLProgram::checkGLError();
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// TODO:monguri:実装
		if (_isC3b) {
			//glUniform3fv(_glProgramForForwardRendering.getUniformLocation("u_cameraPosition"), 1, (GLfloat*)&Director::getCamera().getPosition());
			//GLProgram::checkGLError();

			//glUniform3fv(_glProgramForForwardRendering.getUniformLocation("u_materialAmbient"), 1, (GLfloat*)&_ambient);
			//GLProgram::checkGLError();

			//glUniform3fv(_glProgramForForwardRendering.getUniformLocation("u_materialDiffuse"), 1, (GLfloat*)&_diffuse);
			//GLProgram::checkGLError();

			//glUniform3fv(_glProgramForForwardRendering.getUniformLocation("u_materialSpecular"), 1, (GLfloat*)&_specular);
			//GLProgram::checkGLError();

			//glUniform1f(_glProgramForForwardRendering.getUniformLocation("u_materialShininess"), _shininess);
			//GLProgram::checkGLError();

			//glUniform3fv(_glProgramForForwardRendering.uniformMaterialEmissive, 1, (GLfloat*)&_emissive);
			//GLProgram::checkGLError();

			//glUniform1f(_glProgramForForwardRendering.uniformMaterialOpacity, 1, (GLfloat*)&_emissive);
			//GLProgram::checkGLError();
		}
#endif
	});

	Director::getRenderer().addCommand(&_renderForwardCommand);
}

} // namespace mgrrenderer
