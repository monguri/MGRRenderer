#include "Sprite3D.h"
#include "ObjLoader.h"
#include "Image.h"
#include "Director.h"
#include "Light.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DTexture.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "GLTexture.h"
#endif

namespace mgrrenderer
{

Sprite3D::Sprite3D() :
_isObj(false),
_isC3b(false),
_texture(nullptr),
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

	if (_texture != nullptr)
	{
		delete _texture;
		_texture = nullptr;
	}
}

bool Sprite3D::initWithModel(const std::string& filePath)
{
	_isObj = false;
	_isC3b = false;

	const std::string& ext = filePath.substr(filePath.length() - 4, 4);
	if (ext == ".obj")
	{
		_isObj = true;

		std::vector<ObjLoader::MeshData> meshList;
		std::vector<ObjLoader::MaterialData> materialList;

		const std::string& err = ObjLoader::loadObj(filePath, meshList, materialList);
		if (!err.empty())
		{
			Logger::log(err.c_str());
			return false;
		}

		_vertices.clear();

		// materialListは現状無視
		// TODO:そもそもMeshDataはその時点でマテリアルごとにまとまってないのでは？faceGroupだからまとまってる？
		// →まとまってる。しかし、今回はマテリアルは一種類という前提でいこう
		// 本来は、std::vector<std::vector<Position3DTextureCoordinates>> がメンバ変数になってて、マテリアルごとに切り替えて描画する
		// テクスチャも本来は切り替え前提だからsetTextureってメソッドおかしいよな。。
		Logger::logAssert(meshList.size() == 1, "現状メッシュ複数には対応してない。");
		const ObjLoader::MeshData& mesh = meshList[0];
		_vertices = mesh.vertices;
		_indices = mesh.indices;
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

		C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
		_indices = meshData->subMeshIndices[0];

		_perVertexByteSize = 0;

		for (C3bLoader::MeshVertexAttribute attrib : meshData->attributes)
		{
			_perVertexByteSize += attrib.attributeSizeBytes;
		}

		C3bLoader::MaterialData* materialData = materialDatas->materialDatas[0];
		const C3bLoader::TextureData& texture = materialData->textures[0];
		setTexture(texture.fileName);
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
	ID3D11Device* direct3dDevice = Director::getInstance()->getDirect3dDevice();
	HRESULT result = E_FAIL;

	if (_isObj)
	{
		// 頂点バッファの定義
		D3D11_BUFFER_DESC vertexBufferDesc;
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(Position3DNormalTextureCoordinates) * _vertices.size();
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		// 頂点バッファのサブリソースの定義
		D3D11_SUBRESOURCE_DATA vertexBufferSubData;
		vertexBufferSubData.pSysMem = _vertices.data();
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
		_d3dProgram.addVertexBuffer(vertexBuffer);

		// インデックスバッファの定義
		D3D11_BUFFER_DESC indexBufferDesc;
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(USHORT) * _indices.size();
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		// インデックスバッファのサブリソースの定義
		D3D11_SUBRESOURCE_DATA indexBufferSubData;
		indexBufferSubData.pSysMem = _indices.data();
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

		bool depthEnable = true;
		_d3dProgram.initWithShaderFile("Obj.hlsl", depthEnable);

		// 入力レイアウトオブジェクトの作成
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{D3DProgram::SEMANTIC_POSITION.c_str(), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			//{D3DProgram::SEMANTIC_NORMAL.c_str(), 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(Vec3), D3D11_INPUT_PER_VERTEX_DATA, 0},
			{D3DProgram::SEMANTIC_TEXTURE_COORDINATE.c_str(), 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(Vec3) * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
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
		_d3dProgram.addVertexBuffer(vertexBuffer);

		// インデックスバッファの定義
		D3D11_BUFFER_DESC indexBufferDesc;
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(USHORT) * _indices.size();
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		// インデックスバッファのサブリソースの定義
		D3D11_SUBRESOURCE_DATA indexBufferSubData;
		indexBufferSubData.pSysMem = _indices.data();
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

		bool depthEnable = true;
		_d3dProgram.initWithShaderFile("C3bC3t.hlsl", depthEnable);

		// 入力レイアウトオブジェクトの作成
		std::vector<D3D11_INPUT_ELEMENT_DESC> layouts(meshData->numAttribute);
		for (int i = 0, offset = 0; i < meshData->numAttribute; ++i)
		{
			const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
			D3D11_INPUT_ELEMENT_DESC layout = {attrib.semantic.c_str(), 0, D3DProgram::getDxgiFormat(attrib.semantic), 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0};
			layouts[i] = layout;
			offset += attrib.attributeSizeBytes;
		}

		ID3D11InputLayout* inputLayout = nullptr;
		result = direct3dDevice->CreateInputLayout(
			&layouts[0],
			layouts.size(), 
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
	}

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
	_d3dProgram.addConstantBuffer(constantBuffer);

	// View行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(constantBuffer);

	// Projection行列用
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(constantBuffer);

	constantBufferDesc.ByteWidth = sizeof(Color4F); // getColor()のColor3Bにすると12バイト境界なので16バイト境界のためにパディングデータを作らねばならない
	// 乗算色
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(constantBuffer);

	if (_isC3b)
	{
		// スキニングのマトリックスパレット
		constantBufferDesc.ByteWidth = sizeof(Mat4) * MAX_SKINNING_JOINT;
		ID3D11Buffer* constantBuffer = nullptr;
		result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
		if (FAILED(result))
		{
			Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
			return false;
		}
		_d3dProgram.addConstantBuffer(constantBuffer);
	}
#elif defined(MGRRENDERER_USE_OPENGL)
	if (_isObj)
	{
		_glProgram.initWithShaderString(
			// vertex shader
			// ModelDataしか使わない場合
			"attribute vec4 a_position;"
			"attribute vec4 a_normal;"
			"attribute vec2 a_texCoord;"
			"varying vec4 v_normal;"
			"varying vec2 v_texCoord;"
			"varying vec3 v_vertexToPointLightDirection;"
			"varying vec3 v_vertexToSpotLightDirection;"
			"varying vec4 v_lightPosition;"
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
			"	v_normal = u_normalMatrix * a_normal;" // scale変換に対応するためにモデル行列の逆行列を転置したものを用いる
			"	v_texCoord = a_texCoord;"
			"	v_texCoord.y = 1.0 - v_texCoord.y;" // c3bの事情によるもの
			"	v_lightPosition = u_depthBiasMatrix * u_lightProjectionMatrix * u_lightViewMatrix * worldPosition;"
			"}"
			,
			// fragment shader
			"uniform sampler2D u_texture;"
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
			"varying vec2 v_texCoord;"
			"varying vec3 v_vertexToPointLightDirection;"
			"varying vec3 v_vertexToSpotLightDirection;"
			"varying vec4 v_lightPosition;"
			""
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
			"	gl_FragColor = texture2D(u_texture, v_texCoord) * vec4(u_multipleColor, 1.0) * vec4((diffuseSpecularLightColor.rgb * outShadowFlag + ambientLightColor.rgb), 1.0);" // テクスチャ番号は0のみに対応
			"}"
			);
	}
	else if (_isC3b)
	{
		_glProgram.initWithShaderString(
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
			"attribute vec3 a_position;" // これがvec3になっているのに注意 TODO:なぜなのか？
			"attribute vec3 a_normal;"
			"attribute vec2 a_texCoord;"
			"attribute vec4 a_blendWeight;"
			"attribute vec4 a_blendIndex;"
			""
			"const int MAX_SKINNING_JOINT = 60;" // TODO:なぜ60個までなのか？
			""
			"uniform mat4 u_modelMatrix;"
			"uniform mat4 u_lightViewMatrix;" // 影付けに使うライトをカメラに見立てたビュー行列
			"uniform mat4 u_lightProjectionMatrix;" // 影付けに使うライトをカメラに見立てたプロジェクション行列
			"uniform mat4 u_viewMatrix;"
			"uniform mat4 u_projectionMatrix;"
			"uniform mat4 u_depthBiasMatrix;"
			"uniform mat4 u_normalMatrix;" // scale変換に対応するためにモデル行列の逆行列を転置したものを用いる
			"uniform vec3 u_pointLightPosition;"
			"uniform vec3 u_spotLightPosition;"
			"uniform vec3 u_cameraPosition;"
			"uniform mat4 u_matrixPalette[MAX_SKINNING_JOINT];"
			""
			"varying vec4 v_lightPosition;"
			"varying vec4 v_normal;"
			"varying vec2 v_texCoord;"
			"varying vec3 v_vertexToPointLightDirection;"
			"varying vec3 v_vertexToSpotLightDirection;"
			"varying vec3 v_vertexToCameraDirection;"
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
			"	vec4 worldPosition = u_modelMatrix * getPosition();"
			"	v_vertexToPointLightDirection = u_pointLightPosition - worldPosition.xyz;"
			"	v_vertexToSpotLightDirection = u_spotLightPosition - worldPosition.xyz;"
			"	v_vertexToCameraDirection = u_cameraPosition - worldPosition.xyz;"
			"	gl_Position = u_projectionMatrix * u_viewMatrix * worldPosition;"
			"	vec4 normal = vec4(a_normal, 1.0);"
			"	v_normal = u_normalMatrix * normal;"
			"	v_texCoord = a_texCoord;"
			"	v_texCoord.y = 1.0 - v_texCoord.y;" // c3bの事情によるもの
			"	v_lightPosition = u_depthBiasMatrix * u_lightProjectionMatrix * u_lightViewMatrix * u_modelMatrix * getPosition();"
			"}"
			,
			// fragment shader
			"uniform sampler2D u_texture;"
			"uniform sampler2DShadow u_shadowTexture;"
			"uniform vec3 u_multipleColor;"
			"uniform vec3 u_directionalLightColor;"
			"uniform vec3 u_directionalLightDirection;"
			"uniform vec3 u_ambientLightColor;"
			"uniform vec3 u_pointLightColor;"
			"uniform float u_pointLightRangeInverse;"
			"uniform vec3 u_spotLightColor;"
			"uniform vec3 u_spotLightDirection;"
			"uniform float u_spotLightRangeInverse;"
			"uniform float u_spotLightInnerAngleCos;"
			"uniform float u_spotLightOuterAngleCos;"
			"uniform vec3 u_materialAmbient;"
			"uniform vec3 u_materialDiffuse;"
			"uniform vec3 u_materialSpecular;"
			"uniform float u_materialShininess;"
			"uniform vec3 u_materialEmissive;"
			"uniform float u_materialOpacity;"
			"varying vec4 v_lightPosition;"
			"varying vec4 v_normal;"
			"varying vec2 v_texCoord;"
			"varying vec3 v_vertexToPointLightDirection;"
			"varying vec3 v_vertexToSpotLightDirection;"
			"varying vec3 v_vertexToCameraDirection;"
			""
			"vec3 computeLightedColor(vec3 normalVector, vec3 lightDirection, vec3 cameraDirection, vec3 lightColor, vec3 ambient, vec3 diffuse, vec3 specular, float shininess, float attenuation)"
			"{"
			"	vec3 ambientColor = lightColor * ambient * attenuation;"
			""
			"	float intensityPerUnitArea = max(dot(normalVector, lightDirection), 0.0);"
			""
			"	vec3 diffuseColor = lightColor * diffuse * intensityPerUnitArea * attenuation;"
			""
			"	vec3 reflectedLightDirection = reflect(-lightDirection, normalVector);"
			"	vec3 specularColor = vec3(0.0, 0.0, 0.0);"
			"	if (intensityPerUnitArea > 0.0)"
			"	{"
			"		specularColor = lightColor * specular * pow(max(dot(reflectedLightDirection, cameraDirection), 0.0), shininess);"
			"	}"
			""
			"	return ambientColor + diffuseColor + specularColor;"
			"}"
			""
			"void main()"
			"{"
			"	vec4 ambientLightColor = vec4(u_ambientLightColor, 1.0);"
			""
			"	vec3 normal = normalize(v_normal.xyz);" // データ形式の時点でnormalizeされてない法線がある模様
			""
			"	vec3 cameraDirection = normalize(v_vertexToCameraDirection);"
			""
			"	vec4 diffuseSpecularLightColor = vec4(0.0, 0.0, 0.0, 1.0);"
			"	diffuseSpecularLightColor.rgb += computeLightedColor(normal, -u_directionalLightDirection, cameraDirection, u_directionalLightColor, u_materialAmbient, u_materialDiffuse, u_materialSpecular, u_materialShininess, 1.0);"
			""
			"	vec3 dir = v_vertexToPointLightDirection * u_pointLightRangeInverse;"
			"	float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);"
			"	diffuseSpecularLightColor.rgb += computeLightedColor(normal, normalize(v_vertexToPointLightDirection), cameraDirection, u_pointLightColor, u_materialAmbient, u_materialDiffuse, u_materialSpecular, u_materialShininess, attenuation);"
			""
			"	dir = v_vertexToSpotLightDirection * u_spotLightRangeInverse;"
			"	attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);"
			"	vec3 vertexToSpotLightDirection = normalize(v_vertexToSpotLightDirection);"
			"	float spotCurrentAngleCos = dot(u_spotLightDirection, -vertexToSpotLightDirection);"
			"	attenuation *= smoothstep(u_spotLightOuterAngleCos, u_spotLightInnerAngleCos, spotCurrentAngleCos);"
			"	attenuation = clamp(attenuation, 0.0, 1.0);"
			"	diffuseSpecularLightColor.rgb += computeLightedColor(normal, vertexToSpotLightDirection, cameraDirection, u_spotLightColor, u_materialAmbient, u_materialDiffuse, u_materialSpecular, u_materialShininess, attenuation);"
			""
			"	float outShadowFlag = textureProj(u_shadowTexture, v_lightPosition);"
			"	gl_FragColor = texture2D(u_texture, v_texCoord) * vec4(u_multipleColor, 1.0) * vec4((diffuseSpecularLightColor.rgb * outShadowFlag + ambientLightColor.rgb), 1.0);" // テクスチャ番号は0のみに対応
			"}"
			);
	}

	// TODO:ライトの判定入れないと

	if (_isObj)
	{
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
			"void main()"
			"{" // 何もせずとも深度は自動で書き込まれる 
			"}"
		);
	}
#endif

	return true;
}

void Sprite3D::setTexture(const std::string& filePath)
{
	if (_texture != nullptr)
	{
		delete _texture;
		_texture = nullptr;
	}

	Image image; // ImageはCPU側のメモリを使っているのでこのスコープで解放されてもよいものだからスタックに取る
	image.initWithFilePath(filePath);

	// TextureはGPU側のメモリを使ってるので解放されると困るのでヒープにとる
#if defined(MGRRENDERER_USE_DIRECT3D)
	_texture = new D3DTexture(); 
#elif defined(MGRRENDERER_USE_OPENGL)
	_texture = new GLTexture();
#endif
	static_cast<Texture*>(_texture)->initWithImage(image); // TODO:なぜか暗黙に継承元クラスのメソッドが呼べない
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
	for (int i = 0; i < numSkinJoint; ++i)
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
	for (int i = 0; i < numSkinJoint; ++i)
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

void Sprite3D::renderShadowMap()
{
	_renderShadowMapCommand.init([=]
	{
		Node::renderShadowMap();

#if defined(MGRRENDERER_USE_OPENGL)
		glEnable(GL_DEPTH_TEST);
#endif

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

#if defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		// 行列の設定
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

		// 頂点属性の設定
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_WEIGHT);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_INDEX);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		// TODO:objあるいはc3t/c3bでメッシュデータは一個である前提
		if (_isObj)
		{
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].position);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].normal);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		}
		else if (_isC3b)
		{
			// TODO:objあるいはc3t/c3bでメッシュデータは一個である前提
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (int i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
				offset += attrib.size;
			}
		}

		// スキニングのマトリックスパレットの設定
		if (_isC3b) {
			Logger::logAssert(_matrixPalette.size() > 0, "マトリックスパレットは0でない前提");
			glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette[0].m));
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		}

		glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, _indices.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
#endif
	});

	Director::getRenderer().addCommand(&_renderShadowMapCommand);
}

void Sprite3D::renderWithShadowMap()
{
	_renderCommand.init([=]
	{
		Node::renderWithShadowMap();

#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
		const std::vector<ID3D11Buffer*>& constantBuffers = _d3dProgram.getConstantBuffers();

		// TODO:ここらへん共通化したいな。。
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// モデル行列のマップ
		HRESULT result = direct3dContext->Map(
			constantBuffers[0],
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 modelMatrix = getModelMatrix();
		modelMatrix.transpose();
		CopyMemory(mappedResource.pData, &modelMatrix.m, sizeof(modelMatrix));
		direct3dContext->Unmap(constantBuffers[0], 0);

		// ビュー行列のマップ
		result = direct3dContext->Map(
			constantBuffers[1],
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 viewMatrix = Director::getCamera().getViewMatrix();
		viewMatrix.transpose(); // Direct3Dでは転置した状態で入れる
		CopyMemory(mappedResource.pData, &viewMatrix.m, sizeof(viewMatrix));
		direct3dContext->Unmap(constantBuffers[1], 0);

		// プロジェクション行列のマップ
		result = direct3dContext->Map(
			constantBuffers[2],
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
		direct3dContext->Unmap(constantBuffers[2], 0);

		// 乗算色のマップ
		result = direct3dContext->Map(
			constantBuffers[3],
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		const Color4F& multiplyColor = Color4F(Color4B(getColor().r, getColor().g, getColor().b, 255));
		CopyMemory(mappedResource.pData, &multiplyColor , sizeof(multiplyColor));
		direct3dContext->Unmap(constantBuffers[3], 0);

		if (_isC3b)
		{
			// ジョイントマトリックスパレットのマップ
			result = direct3dContext->Map(
				constantBuffers[4],
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&mappedResource
			);
			Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
			CopyMemory(mappedResource.pData, _matrixPalette.data(), sizeof(Mat4) * _matrixPalette.size());
			direct3dContext->Unmap(constantBuffers[4], 0);
		}

		UINT strides[1];
		if (_isObj)
		{
			strides[0] = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			strides[0] = _perVertexByteSize;
		}

		int numConstantBuffer = _isC3b ? 5 : 4;
		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgram.getVertexBuffers().size(), _d3dProgram.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgram.getIndexBuffer(), DXGI_FORMAT_R16_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgram.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgram.setShadersToDirect3DContext(direct3dContext);
		_d3dProgram.setConstantBuffersToDirect3DContext(direct3dContext);

		direct3dContext->RSSetState(_d3dProgram.getRasterizeState());

		ID3D11ShaderResourceView* resourceView = _texture->getShaderResourceView(); //TODO:型変換がうまくいかないので一度変数に代入している
		direct3dContext->PSSetShaderResources(0, 1, &resourceView);
		ID3D11SamplerState* samplerState = _texture->getSamplerState();
		direct3dContext->PSSetSamplers(0, 1, &samplerState); //TODO:型変換がうまくいかないので一度変数に代入している

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgram.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->OMSetDepthStencilState(_d3dProgram.getDepthStancilState(), 0);

		direct3dContext->DrawIndexed(_indices.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glEnable(GL_DEPTH_TEST);

		// cocos2d-xはTriangleCommand発行してる形だからな。。テクスチャバインドはTexture2Dでやってるのに大丈夫か？
		glUseProgram(_glProgram.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniform3f(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		// 行列の設定
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
					glBindTexture(GL_TEXTURE_2D, dirLight->getShadowMapData().textureId);
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
			default:
				break;
			}
		}

		// 頂点属性の設定
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::NORMAL);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_WEIGHT);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_INDEX);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		if (_isObj)
		{
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].position);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].normal);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		}
		else if (_isC3b)
		{
			// TODO:objあるいはc3t/c3bでメッシュデータは一個である前提
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (int i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
				offset += attrib.size;
			}
		}

		// スキニングのマトリックスパレットの設定
		if (_isC3b) {
			Logger::logAssert(_matrixPalette.size() > 0, "マトリックスパレットは0でない前提");
			glUniformMatrix4fv(_glProgram.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette.data()));
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		}

		// TODO:monguri:実装
		if (_isC3b) {
			glUniform3fv(_glProgram.getUniformLocation("u_cameraPosition"), 1, (GLfloat*)&Director::getCamera().getPosition());
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform3fv(_glProgram.getUniformLocation("u_materialAmbient"), 1, (GLfloat*)&_ambient);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform3fv(_glProgram.getUniformLocation("u_materialDiffuse"), 1, (GLfloat*)&_diffuse);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform3fv(_glProgram.getUniformLocation("u_materialSpecular"), 1, (GLfloat*)&_specular);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			glUniform1f(_glProgram.getUniformLocation("u_materialShininess"), _shininess);
			Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			//glUniform3fv(_glProgram.uniformMaterialEmissive, 1, (GLfloat*)&_emissive);
			//Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

			//glUniform1f(_glProgram.uniformMaterialOpacity, 1, (GLfloat*)&_emissive);
			//Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		}

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, _indices.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glBindTexture(GL_TEXTURE_2D, 0);
#endif
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
