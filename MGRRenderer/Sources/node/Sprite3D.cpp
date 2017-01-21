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

		// materialList�͌��󖳎�
		// TODO:��������MeshData�͂��̎��_�Ń}�e���A�����Ƃɂ܂Ƃ܂��ĂȂ��̂ł́HfaceGroup������܂Ƃ܂��Ă�H
		// ���܂Ƃ܂��Ă�B�������A����̓}�e���A���͈��ނƂ����O��ł�����
		// �{���́Astd::vector<std::vector<Position3DTextureCoordinates>> �������o�ϐ��ɂȂ��ĂāA�}�e���A�����Ƃɐ؂�ւ��ĕ`�悷��
		// �e�N�X�`�����{���͐؂�ւ��O�񂾂���setTexture���ă��\�b�h����������ȁB�B
		Logger::logAssert(meshList.size() == 1, "���󃁃b�V�������ɂ͑Ή����ĂȂ��B");
		const ObjLoader::MeshData& mesh = meshList[0];
		_vertices = mesh.vertices;
		_indices = mesh.indices;
	}
	else if (ext == ".c3t" || ext == ".c3b")
	{
		_isC3b = true; // TODO:���̃t���O�����͔��ɂ�����

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
		Logger::logAssert(false, "�Ή����ĂȂ��g���q%s", ext);
	}

#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11Device* direct3dDevice = Director::getInstance()->getDirect3dDevice();
	HRESULT result = E_FAIL;

	if (_isObj)
	{
		// ���_�o�b�t�@�̒�`
		D3D11_BUFFER_DESC vertexBufferDesc;
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(Position3DNormalTextureCoordinates) * _vertices.size();
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		// ���_�o�b�t�@�̃T�u���\�[�X�̒�`
		D3D11_SUBRESOURCE_DATA vertexBufferSubData;
		vertexBufferSubData.pSysMem = _vertices.data();
		vertexBufferSubData.SysMemPitch = 0;
		vertexBufferSubData.SysMemSlicePitch = 0;

		// ���_�o�b�t�@�̃T�u���\�[�X�̍쐬
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

		// �C���f�b�N�X�o�b�t�@�̒�`
		D3D11_BUFFER_DESC indexBufferDesc;
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(USHORT) * _indices.size();
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		// �C���f�b�N�X�o�b�t�@�̃T�u���\�[�X�̒�`
		D3D11_SUBRESOURCE_DATA indexBufferSubData;
		indexBufferSubData.pSysMem = _indices.data();
		indexBufferSubData.SysMemPitch = 0;
		indexBufferSubData.SysMemSlicePitch = 0;

		// �C���f�b�N�X�o�b�t�@�̃T�u���\�[�X�̍쐬
		ID3D11Buffer* indexBuffer = nullptr;
		result = direct3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferSubData, &indexBuffer);
		if (FAILED(result))
		{
			Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
			return false;
		}
		_d3dProgramForForwardRendering.setIndexBuffer(indexBuffer);
		_d3dProgramForShadowMap.setIndexBuffer(indexBuffer);
		_d3dProgramForPointLightShadowMap.setIndexBuffer(indexBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
		_d3dProgramForGBuffer.setIndexBuffer(indexBuffer);
#endif

		bool depthEnable = true;
		_d3dProgramForForwardRendering.initWithShaderFile("Resources/shader/Obj.hlsl", depthEnable, "VS", "", "PS");
		_d3dProgramForShadowMap.initWithShaderFile("Resources/shader/Obj.hlsl", depthEnable, "VS_SM", "", "");
		_d3dProgramForPointLightShadowMap.initWithShaderFile("Resources/shader/Obj.hlsl", depthEnable, "VS_SM_POINT_LIGHT", "GS_SM_POINT_LIGHT", "");
#if defined(MGRRENDERER_DEFERRED_RENDERING)
		_d3dProgramForGBuffer.initWithShaderFile("Resources/shader/Obj.hlsl", depthEnable, "VS_GBUFFER", "", "PS_GBUFFER");
#endif

		// ���̓��C�A�E�g�I�u�W�F�N�g�̍쐬
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

		// ���_�o�b�t�@�̒�`
		D3D11_BUFFER_DESC vertexBufferDesc;
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(float) * meshData->vertices.size();
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		// ���_�o�b�t�@�̃T�u���\�[�X�̒�`
		D3D11_SUBRESOURCE_DATA vertexBufferSubData;
		vertexBufferSubData.pSysMem = &meshData->vertices[0];
		vertexBufferSubData.SysMemPitch = 0;
		vertexBufferSubData.SysMemSlicePitch = 0;

		// ���_�o�b�t�@�̃T�u���\�[�X�̍쐬
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

		// �C���f�b�N�X�o�b�t�@�̒�`
		D3D11_BUFFER_DESC indexBufferDesc;
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(USHORT) * _indices.size();
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		// �C���f�b�N�X�o�b�t�@�̃T�u���\�[�X�̒�`
		D3D11_SUBRESOURCE_DATA indexBufferSubData;
		indexBufferSubData.pSysMem = _indices.data();
		indexBufferSubData.SysMemPitch = 0;
		indexBufferSubData.SysMemSlicePitch = 0;

		// �C���f�b�N�X�o�b�t�@�̃T�u���\�[�X�̍쐬
		ID3D11Buffer* indexBuffer = nullptr;
		result = direct3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferSubData, &indexBuffer);
		if (FAILED(result))
		{
			Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
			return false;
		}
		_d3dProgramForForwardRendering.setIndexBuffer(indexBuffer);
		_d3dProgramForShadowMap.setIndexBuffer(indexBuffer);
		_d3dProgramForPointLightShadowMap.setIndexBuffer(indexBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
		_d3dProgramForGBuffer.setIndexBuffer(indexBuffer);
#endif

		bool depthEnable = true;
		_d3dProgramForForwardRendering.initWithShaderFile("Resources/shader/C3bC3t.hlsl", depthEnable, "VS", "", "PS");
		_d3dProgramForShadowMap.initWithShaderFile("Resources/shader/C3bC3t.hlsl", depthEnable, "VS_SM", "", "");
		_d3dProgramForPointLightShadowMap.initWithShaderFile("Resources/shader/C3bC3t.hlsl", depthEnable, "VS_SM_POINT_LIGHT", "GS_SM_POINT_LIGHT", "");
#if defined(MGRRENDERER_DEFERRED_RENDERING)
		_d3dProgramForGBuffer.initWithShaderFile("Resources/shader/C3bC3t.hlsl", depthEnable, "VS_GBUFFER", "", "PS_GBUFFER");
#endif

		// ���̓��C�A�E�g�I�u�W�F�N�g�̍쐬
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

	// �萔�o�b�t�@�̍쐬
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;
	constantBufferDesc.ByteWidth = sizeof(Mat4);

	// Model�s��p
	ID3D11Buffer* constantBuffer = nullptr;
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

	// View�s��p
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

	// Projection�s��p
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

	// �f�v�X�o�C�A�X�s��p
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

	// Normal�s��p
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_NORMAL_MATRIX, constantBuffer);
#endif

	constantBufferDesc.ByteWidth = sizeof(Color4F); // getColor()��Color3B�ɂ����12�o�C�g���E�Ȃ̂�16�o�C�g���E�̂��߂Ƀp�f�B���O�f�[�^�����˂΂Ȃ�Ȃ�
	// ��Z�F
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#endif

	// �A���r�G���g���C�g�J���[
	constantBufferDesc.ByteWidth = sizeof(AmbientLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#endif

	// �f�B���N�V���i���g���C�gView�s��p
	constantBufferDesc.ByteWidth = sizeof(Mat4);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#endif

	// �f�B���N�V���i���g���C�gProjection�s��p
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#endif

	// �f�B���N�V���i���g���C�g�p�����[�^�[
	constantBufferDesc.ByteWidth = sizeof(DirectionalLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#endif

	// �|�C���g���C�g�p�����[�^�[
	constantBufferDesc.ByteWidth = sizeof(PointLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#endif

	// �X�|�b�g���C�g�p�����[�^�[
	constantBufferDesc.ByteWidth = sizeof(SpotLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER, constantBuffer);
	_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
	_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER, constantBuffer); // �V�F�[�_�ł͎g��Ȃ����A�C���f�b�N�X�̐��l�����L���Ă���̂ł���Ȃ��悤�ɃV���h�E�}�b�v�p�萔�o�b�t�@�ɂ�������
#endif

	if (_isC3b)
	{
		// �X�L�j���O�̃}�g���b�N�X�p���b�g
		constantBufferDesc.ByteWidth = sizeof(Mat4) * MAX_SKINNING_JOINT;
		constantBuffer = nullptr;
		result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
		if (FAILED(result))
		{
			Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
			return false;
		}
		_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE, constantBuffer);
		_d3dProgramForShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE, constantBuffer);
		_d3dProgramForPointLightShadowMap.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE, constantBuffer);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
		_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_JOINT_MATRIX_PALLETE, constantBuffer);
#endif
	}

#elif defined(MGRRENDERER_USE_OPENGL)
	if (_isObj)
	{
		_glProgram.initWithShaderString(
			// vertex shader
			// ModelData�����g��Ȃ��ꍇ
			"#version 430\n"
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
			"uniform mat4 u_lightViewMatrix;" // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��r���[�s��
			"uniform mat4 u_lightProjectionMatrix;" // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��v���W�F�N�V�����s��
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
			"	v_normal = vec4(normalize((u_normalMatrix * a_normal).xyz), 1.0);" // scale�ϊ��ɑΉ����邽�߂Ƀ��f���s��̋t�s���]�u�������̂�p����
			"	v_texCoord = a_texCoord;"
			"	v_texCoord.y = 1.0 - v_texCoord.y;" // c3b�̎���ɂ�����
			"	v_lightPosition = u_depthBiasMatrix * u_lightProjectionMatrix * u_lightViewMatrix * worldPosition;"
			"}"
			,
			// fragment shader
			"#version 430\n"
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
			"	vec3 normal = normalize(v_normal.xyz);" // �f�[�^�`���̎��_��normalize����ĂȂ��@��������͗l
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
			"	gl_FragColor = texture2D(u_texture, v_texCoord) * vec4(u_multipleColor, 1.0) * vec4((diffuseSpecularLightColor.rgb * outShadowFlag + ambientLightColor.rgb), 1.0);" // �e�N�X�`���ԍ���0�݂̂ɑΉ�
			"}"
			);
	}
	else if (_isC3b)
	{
		_glProgram.initWithShaderString(
			// vertex shader
			// ModelData�����g��Ȃ��ꍇ
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
			//// �A�j���[�V�������g���ꍇ
			"#version 430\n"
			"attribute vec3 a_position;" // ���ꂪvec3�ɂȂ��Ă���̂ɒ��� TODO:�Ȃ��Ȃ̂��H
			"attribute vec3 a_normal;"
			"attribute vec2 a_texCoord;"
			"attribute vec4 a_blendWeight;"
			"attribute vec4 a_blendIndex;"
			""
			"const int MAX_SKINNING_JOINT = 60;" // TODO:�Ȃ�60�܂łȂ̂��H
			""
			"uniform mat4 u_modelMatrix;"
			"uniform mat4 u_lightViewMatrix;" // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��r���[�s��
			"uniform mat4 u_lightProjectionMatrix;" // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��v���W�F�N�V�����s��
			"uniform mat4 u_viewMatrix;"
			"uniform mat4 u_projectionMatrix;"
			"uniform mat4 u_depthBiasMatrix;"
			"uniform mat4 u_normalMatrix;" // scale�ϊ��ɑΉ����邽�߂Ƀ��f���s��̋t�s���]�u�������̂�p����
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
			"	v_normal = vec4(normalize((u_normalMatrix * normal).xyz), 1.0);"
			"	v_texCoord = a_texCoord;"
			"	v_texCoord.y = 1.0 - v_texCoord.y;" // c3b�̎���ɂ�����
			"	v_lightPosition = u_depthBiasMatrix * u_lightProjectionMatrix * u_lightViewMatrix * u_modelMatrix * getPosition();"
			"}"
			,
			// fragment shader
			"#version 430\n"
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
			"	vec3 normal = normalize(v_normal.xyz);" // �f�[�^�`���̎��_��normalize����ĂȂ��@��������͗l
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
			"	gl_FragColor = texture2D(u_texture, v_texCoord) * vec4(u_multipleColor, 1.0) * vec4((diffuseSpecularLightColor.rgb * outShadowFlag + ambientLightColor.rgb), 1.0);" // �e�N�X�`���ԍ���0�݂̂ɑΉ�
			"}"
			);
	}

	// TODO:���C�g�̔������Ȃ���

	if (_isObj)
	{
		_glProgramForShadowMap.initWithShaderString(
			// vertex shader
			// ModelData�����g��Ȃ��ꍇ
			"#version 430\n"
			"attribute vec4 a_position;"
			"uniform mat4 u_modelMatrix;"
			"uniform mat4 u_lightViewMatrix;" // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��r���[�s��
			"uniform mat4 u_lightProjectionMatrix;"
			"void main()"
			"{"
			"	gl_Position = u_lightProjectionMatrix * u_lightViewMatrix * u_modelMatrix * a_position;"
			"}"
			,
			// fragment shader
			"#version 430\n"
			"void main()"
			"{" // ���������Ƃ��[�x�͎����ŏ������܂�� 
			"}"
		);
	}
	else if (_isC3b)
	{
		_glProgramForShadowMap.initWithShaderString(
			// vertex shader
			// ModelData�����g��Ȃ��ꍇ
			//"attribute vec4 a_position;"
			//"uniform mat4 u_lightViewMatrix;" // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��r���[�s��
			//"uniform mat4 u_viewMatrix;"
			//"uniform mat4 u_projectionMatrix;"
			//"void main()"
			//"{"
			//"	gl_Position = u_projectionMatrix * u_lightViewMatrix * u_modelMatrix * getPosition();"
			//"}"
			//// �A�j���[�V�������g���ꍇ
			"#version 430\n"
			"attribute vec3 a_position;" // ���ꂪvec3�ɂȂ��Ă���̂ɒ��� TODO:�Ȃ��Ȃ̂��H
			"attribute vec4 a_blendWeight;"
			"attribute vec4 a_blendIndex;"
			""
			"const int MAX_SKINNING_JOINT = 60;" // TODO:�Ȃ�60�܂łȂ̂��H
			""
			"uniform mat4 u_modelMatrix;"
			"uniform mat4 u_lightViewMatrix;" // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��r���[�s��
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
			"{" // ���������Ƃ��[�x�͎����ŏ������܂�� 
			"}"
		);
	}

	if (_isObj)
	{
		// STRINGIFY�ɂ��ǂݍ��݂��ƁAGeForce850M�����܂�#version�̍s�̉��s��ǂݎ���Ă��ꂸGLSL�R���p�C���G���[�ɂȂ�
		_glProgramForGBuffer.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderObj.glsl", "../MGRRenderer/Resources/shader/FragmentShaderPositionNormalTextureMultiplyColorGBuffer.glsl");
	}
	else if (_isC3b)
	{
		_glProgramForGBuffer.initWithShaderString(
			// vertex shader
			// ModelData�����g��Ȃ��ꍇ
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
			//// �A�j���[�V�������g���ꍇ
			"#version 430\n"
			""
			"in vec3 a_position;" // ���ꂪvec3�ɂȂ��Ă���̂ɒ��� TODO:�Ȃ��Ȃ̂��H
			"in vec3 a_normal;"
			"in vec2 a_texCoord;"
			"in vec4 a_blendWeight;"
			"in vec4 a_blendIndex;"
			""
			"const int MAX_SKINNING_JOINT = 60;" // TODO:�Ȃ�60�܂łȂ̂��H
			""
			"uniform mat4 u_modelMatrix;"
			"uniform mat4 u_viewMatrix;"
			"uniform mat4 u_projectionMatrix;"
			"uniform mat4 u_normalMatrix;" // scale�ϊ��ɑΉ����邽�߂Ƀ��f���s��̋t�s���]�u�������̂�p����
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
			"	v_texCoord.y = 1.0 - v_texCoord.y;" // c3b�̎���ɂ�����
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
			"layout (location = 0) out vec4 FragColor;" // �f�v�X�o�b�t�@�̕�
			"layout (location = 1) out vec4 ColorSpecularIntensity;"
			"layout (location = 2) out vec4 Normal;"
			"layout (location = 3) out vec4 SpecularPower;"
			""
			"void main()"
			"{"
			"	"// G�o�b�t�@�ւ̃p�b�L���O���s��
			""
			"	"// specular�͍��̂Ƃ���Ή����ĂȂ�
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

void Sprite3D::setTexture(const std::string& filePath)
{
	if (_texture != nullptr)
	{
		delete _texture;
		_texture = nullptr;
	}

	Image image; // Image��CPU���̃��������g���Ă���̂ł��̃X�R�[�v�ŉ������Ă��悢���̂�����X�^�b�N�Ɏ��
	image.initWithFilePath(filePath);

	// Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
#if defined(MGRRENDERER_USE_DIRECT3D)
	_texture = new D3DTexture(); 
#elif defined(MGRRENDERER_USE_OPENGL)
	_texture = new GLTexture();
#endif
	static_cast<Texture*>(_texture)->initWithImage(image); // TODO:�Ȃ����ÖقɌp�����N���X�̃��\�b�h���ĂׂȂ�
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
			// ���������炻���Ԃ��B���[�v���I���B
			return child;
		}
		else if (child->children.size() > 0)
		{
			// �����炸�A�q������Ȃ�q��T���ɍs��
			C3bLoader::NodeData* findResult = findJointByName(jointName, child->children);
			if (findResult != nullptr)
			{
				// ���������炻���Ԃ��B���[�v���I���B
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

	float t = 0; // 0<=t<=1�̃A�j���[�V�����⊮�p�����[�^
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

	// �A�j���[�V�������s���BAnimate3D::update���Q�l�� C3bLoader::AnimationData�̎g�����Ȃ����̂�
	size_t numSkinJoint = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones.size();
	Logger::logAssert(numSkinJoint == _nodeDatas->nodes[0]->modelNodeDatas[0]->invBindPose.size(), "�W���C���g���͈�v����͂�");

	// ��Ɋe�W���C���g�̃A�j���[�V�����s����쐬����
	for (size_t i = 0; i < numSkinJoint; ++i)
	{
		const std::string& jointName = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones[i];//TODO: nodes���ɗv�f�͈�Aparts���ɂ�������ł��邱�Ƃ�O��ɂ��Ă���

		// �L�[�t���[����񂪂Ȃ�������ANodeDatas::skeletons::transform�̕��̃c���[����ċA�I�ɃW���C���g�ʒu���W�ϊ��z����v�Z����
		// �܂��͑Ή�����{�[����T���A�ݒ肳��Ă���s����擾����
		C3bLoader::NodeData* node = findJointByName(jointName, _nodeDatas->skeleton);
		Logger::logAssert(node != nullptr, "nodes�̕��ɂ��������̂�skeletons����{�[�����Ō����������Ȃ��B");

		// ��U�A�A�j���[�V�����g��Ȃ����ɏ�����
		node->animatedTransform = Mat4::ZERO;

		if (_currentAnimation != nullptr && _currentAnimation->translationKeyFrames.find(jointName) != _currentAnimation->translationKeyFrames.end())
		{
			// �A�j���[�V�������ŁA�f�[�^�ɂ��̃{�[���̃L�[�t���[����񂪂������Ƃ��͂����炩�������s��ŏ㏑������
			const Vec3& translation = _currentAnimation->evaluateTranslation(jointName, t);
			const Quaternion& rotation = _currentAnimation->evaluateRotation(jointName, t);
			const Vec3& scale = _currentAnimation->evaluateScale(jointName, t);

			node->animatedTransform = Mat4::createTransform(translation, rotation, scale);
		}
	}

	// ���ɃW���C���g�̃}�g���b�N�X�p���b�g�����ׂċ��߂�
	for (size_t i = 0; i < numSkinJoint; ++i)
	{
		const std::string& jointName = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones[i];//TODO: nodes���ɗv�f�͈�Aparts���ɂ�������ł��邱�Ƃ�O��ɂ��Ă���

		// �L�[�t���[����񂪂Ȃ�������ANodeDatas::skeletons::transform�̕��̃c���[����ċA�I�ɃW���C���g�ʒu���W�ϊ��z����v�Z����
		// �܂��͑Ή�����{�[����T���A�ݒ肳��Ă���s����擾����
		C3bLoader::NodeData* node = findJointByName(jointName, _nodeDatas->skeleton);
		if (node == nullptr)
		{
			Logger::log("nodes�̕��ɂ��������̂�skeletons����{�[�����Ō����������Ȃ��B");
		}
		Logger::logAssert(node != nullptr, "c3b�̎d�l��A�W���C���g����������Ȃ��͂����Ȃ��B");

		Mat4 transform = (node->animatedTransform != Mat4::ZERO) ? node->animatedTransform : node->transform;

		// �e�����[�g�܂ł����̂ڂ��ă��[���h�s����v�Z����
		while (node->parent != nullptr)
		{
			// TODO:�e�̃{�[�����A�j���[�V�������Ă�\�����������߂ɁANodeData�ɂ������Matrix��������Ƃ��������ȕ��@�B�{����Skeleton3D��Bone3D�݂����ɕʂ̊K�w�\���f�[�^��ێ�������������
			const Mat4& parentTransform = (node->parent->animatedTransform != Mat4::ZERO) ? node->parent->animatedTransform : node->parent->transform;
			transform = parentTransform * transform;
			node = node->parent;
		}

		// �{�[���̔z�u�s��
		const Mat4& invBindPose = _nodeDatas->nodes[0]->modelNodeDatas[0]->invBindPose[i];//TODO: nodes���ɗv�f�͈�Aparts���ɂ�������ł��邱�Ƃ�O��ɂ��Ă���

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
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();

		// TODO:������ւ񋤒ʉ��������ȁB�B
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// ���f���s��̃}�b�v
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

		// �r���[�s��̃}�b�v
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

		// �v���W�F�N�V�����s��̃}�b�v
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

		// �m�[�}���s��̃}�b�v
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

		// ��Z�F�̃}�b�v
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
			// �W���C���g�}�g���b�N�X�p���b�g�̃}�b�v
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

		UINT strides[1];
		if (_isObj)
		{
			strides[0] = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			strides[0] = _perVertexByteSize;
		}

		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForGBuffer.getVertexBuffers().size(), _d3dProgramForGBuffer.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForGBuffer.getIndexBuffer(), DXGI_FORMAT_R16_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForGBuffer.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForGBuffer.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForGBuffer.setConstantBuffersToDirect3DContext(direct3dContext);

		ID3D11ShaderResourceView* resourceView[1] = { _texture->getShaderResourceView() };
		direct3dContext->PSSetShaderResources(0, 1, resourceView);
		ID3D11SamplerState* samplerState[1] = { Director::getRenderer().getLinearSamplerState() };
		direct3dContext->PSSetSamplers(0, 1, samplerState);

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgramForGBuffer.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->DrawIndexed(_indices.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForGBuffer.getShaderProgram());
		GLProgram::checkGLError();

		glUniform3f(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		GLProgram::checkGLError();

		// �s��̐ݒ�
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		GLProgram::checkGLError();

		Mat4 normalMatrix = Mat4::createNormalMatrix(getModelMatrix());
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_NORMAL_MATRIX), 1, GL_FALSE, (GLfloat*)&normalMatrix.m);

		// ���_�����̐ݒ�
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
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].position);
			GLProgram::checkGLError();
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].normal);
			GLProgram::checkGLError();
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);
			GLProgram::checkGLError();
		}
		else if (_isC3b)
		{
			// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				GLProgram::checkGLError();
				offset += attrib.size;
			}
		}

		// �X�L�j���O�̃}�g���b�N�X�p���b�g�̐ݒ�
		if (_isC3b) {
			Logger::logAssert(_matrixPalette.size() > 0, "�}�g���b�N�X�p���b�g��0�łȂ��O��");
			glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette.data()));
			GLProgram::checkGLError();
		}

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		GLProgram::checkGLError();

		glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, _indices.data());
		GLProgram::checkGLError();
		glBindTexture(GL_TEXTURE_2D, 0);
#endif
	});

	Director::getRenderer().addCommand(&_renderGBufferCommand);
}
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

void Sprite3D::renderDirectionalLightShadowMap(const DirectionalLight* light)
{
	Logger::logAssert(light != nullptr, "���C�g������null�łȂ��O��B");
	Logger::logAssert(light->hasShadowMap(), "���C�g�̓V���h�E�������Ă���O��B");

	_renderDirectionalLightShadowMapCommand.init([=]
	{
		Mat4 lightViewMatrix = light->getShadowMapData().viewMatrix;
		Mat4 lightProjectionMatrix = light->getShadowMapData().projectionMatrix;

#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// ���f���s��̃}�b�v
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

		// �r���[�s��̃}�b�v
		result = direct3dContext->Map(
			_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		lightViewMatrix.transpose(); // Direct3D�ł͓]�u������Ԃœ����
		CopyMemory(mappedResource.pData, &lightViewMatrix.m, sizeof(lightViewMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX), 0);

		// �v���W�F�N�V�����s��̃}�b�v
		result = direct3dContext->Map(
			_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		lightProjectionMatrix = Mat4::CHIRARITY_CONVERTER * lightProjectionMatrix; // ����n�ϊ��s��̓v���W�F�N�V�����s��ɍŏ����炩���Ă���
		lightProjectionMatrix.transpose();
		CopyMemory(mappedResource.pData, &lightProjectionMatrix.m, sizeof(lightProjectionMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

		if (_isC3b)
		{
			// �W���C���g�}�g���b�N�X�p���b�g�̃}�b�v
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

		UINT strides[1];
		if (_isObj)
		{
			strides[0] = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			strides[0] = _perVertexByteSize;
		}

		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForShadowMap.getVertexBuffers().size(), _d3dProgramForShadowMap.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForShadowMap.getIndexBuffer(), DXGI_FORMAT_R16_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForShadowMap.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForShadowMap.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForShadowMap.setConstantBuffersToDirect3DContext(direct3dContext);

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgramForShadowMap.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->DrawIndexed(_indices.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		GLProgram::checkGLError();

		// �s��̐ݒ�
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

		// ���_�����̐ݒ�
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_WEIGHT);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_INDEX);
		GLProgram::checkGLError();

		// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
		if (_isObj)
		{
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].position);
			GLProgram::checkGLError();
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].normal);
			GLProgram::checkGLError();
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);
			GLProgram::checkGLError();
		}
		else if (_isC3b)
		{
			// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				GLProgram::checkGLError();
				offset += attrib.size;
			}
		}

		// �X�L�j���O�̃}�g���b�N�X�p���b�g�̐ݒ�
		if (_isC3b) {
			Logger::logAssert(_matrixPalette.size() > 0, "�}�g���b�N�X�p���b�g��0�łȂ��O��");
			glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette[0].m));
			GLProgram::checkGLError();
		}

		glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, _indices.data());
		GLProgram::checkGLError();
#endif
	});

	Director::getRenderer().addCommand(&_renderDirectionalLightShadowMapCommand);
}

void Sprite3D::renderPointLightShadowMap(size_t index, const PointLight* light, CubeMapFace face)
{
	Logger::logAssert(light != nullptr, "���C�g������null�łȂ��O��B");
	Logger::logAssert(light->hasShadowMap(), "���C�g�̓V���h�E�������Ă���O��B");

	_renderPointLightShadowMapCommandList[index][(size_t)face].init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		(void)face;
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// ���f���s��̃}�b�v
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

		// �r���[�s��ƃv���W�F�N�V�����s��̃}�b�v
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
			// �W���C���g�}�g���b�N�X�p���b�g�̃}�b�v
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

		UINT strides[1];
		if (_isObj)
		{
			strides[0] = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			strides[0] = _perVertexByteSize;
		}

		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForPointLightShadowMap.getVertexBuffers().size(), _d3dProgramForPointLightShadowMap.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForPointLightShadowMap.getIndexBuffer(), DXGI_FORMAT_R16_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForPointLightShadowMap.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForPointLightShadowMap.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForPointLightShadowMap.setConstantBuffersToDirect3DContext(direct3dContext);

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgramForPointLightShadowMap.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->DrawIndexed(_indices.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		GLProgram::checkGLError();

		// �s��̐ݒ�
		glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);

		Mat4 lightViewMatrix = light->getShadowMapData().viewMatrices[(int)face];
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

		// ���_�����̐ݒ�
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_WEIGHT);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_INDEX);
		GLProgram::checkGLError();

		// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
		if (_isObj)
		{
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].position);
			GLProgram::checkGLError();
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].normal);
			GLProgram::checkGLError();
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);
			GLProgram::checkGLError();
		}
		else if (_isC3b)
		{
			// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				GLProgram::checkGLError();
				offset += attrib.size;
			}
		}

		// �X�L�j���O�̃}�g���b�N�X�p���b�g�̐ݒ�
		if (_isC3b) {
			Logger::logAssert(_matrixPalette.size() > 0, "�}�g���b�N�X�p���b�g��0�łȂ��O��");
			glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette[0].m));
			GLProgram::checkGLError();
		}

		glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, _indices.data());
		GLProgram::checkGLError();
#endif
	});

	Director::getRenderer().addCommand(&_renderPointLightShadowMapCommandList[index][(size_t)face]);
}

void Sprite3D::renderSpotLightShadowMap(size_t index, const SpotLight* light)
{
	Logger::logAssert(light != nullptr, "���C�g������null�łȂ��O��B");
	Logger::logAssert(light->hasShadowMap(), "���C�g�̓V���h�E�������Ă���O��B");

	_renderSpotLightShadowMapCommandList[index].init([=]
	{
		Mat4 lightViewMatrix = light->getShadowMapData().viewMatrix;
		Mat4 lightProjectionMatrix = light->getShadowMapData().projectionMatrix;

#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// ���f���s��̃}�b�v
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

		// �r���[�s��̃}�b�v
		result = direct3dContext->Map(
			_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		lightViewMatrix.transpose(); // Direct3D�ł͓]�u������Ԃœ����
		CopyMemory(mappedResource.pData, &lightViewMatrix.m, sizeof(lightViewMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX), 0);

		// �v���W�F�N�V�����s��̃}�b�v
		result = direct3dContext->Map(
			_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		lightProjectionMatrix = Mat4::CHIRARITY_CONVERTER * lightProjectionMatrix; // ����n�ϊ��s��̓v���W�F�N�V�����s��ɍŏ����炩���Ă���
		lightProjectionMatrix.transpose();
		CopyMemory(mappedResource.pData, &lightProjectionMatrix.m, sizeof(lightProjectionMatrix));
		direct3dContext->Unmap(_d3dProgramForShadowMap.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

		if (_isC3b)
		{
			// �W���C���g�}�g���b�N�X�p���b�g�̃}�b�v
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

		UINT strides[1];
		if (_isObj)
		{
			strides[0] = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			strides[0] = _perVertexByteSize;
		}

		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForShadowMap.getVertexBuffers().size(), _d3dProgramForShadowMap.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForShadowMap.getIndexBuffer(), DXGI_FORMAT_R16_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForShadowMap.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForShadowMap.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForShadowMap.setConstantBuffersToDirect3DContext(direct3dContext);

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgramForShadowMap.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->DrawIndexed(_indices.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForShadowMap.getShaderProgram());
		GLProgram::checkGLError();

		// �s��̐ݒ�
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

		// ���_�����̐ݒ�
		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_WEIGHT);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::BLEND_INDEX);
		GLProgram::checkGLError();

		// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
		if (_isObj)
		{
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].position);
			GLProgram::checkGLError();
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].normal);
			GLProgram::checkGLError();
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);
			GLProgram::checkGLError();
		}
		else if (_isC3b)
		{
			// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				GLProgram::checkGLError();
				offset += attrib.size;
			}
		}

		// �X�L�j���O�̃}�g���b�N�X�p���b�g�̐ݒ�
		if (_isC3b) {
			Logger::logAssert(_matrixPalette.size() > 0, "�}�g���b�N�X�p���b�g��0�łȂ��O��");
			glUniformMatrix4fv(_glProgramForShadowMap.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette[0].m));
			GLProgram::checkGLError();
		}

		glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, _indices.data());
		GLProgram::checkGLError();
#endif
	});

	Director::getRenderer().addCommand(&_renderSpotLightShadowMapCommandList[index]);
}

void Sprite3D::renderForward()
{
	_renderForwardCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();

		// TODO:������ւ񋤒ʉ��������ȁB�B
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// ���f���s��̃}�b�v
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

		// �r���[�s��̃}�b�v
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

		// �v���W�F�N�V�����s��̃}�b�v
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

		result = direct3dContext->Map(
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 depthBiasMatrix = (Mat4::TEXTURE_COORDINATE_CONVERTER * Mat4::createScale(Vec3(0.5f, 0.5f, 1.0f)) * Mat4::createTranslation(Vec3(1.0f, -1.0f, 0.0f))).transpose(); //TODO: Mat4���Q�ƌ^�ɂ���ƒl�����������Ȃ��Ă��܂�
		CopyMemory(mappedResource.pData, &depthBiasMatrix.m, sizeof(depthBiasMatrix));
		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DEPTH_BIAS_MATRIX), 0);

		// �m�[�}���s��̃}�b�v
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

		// ��Z�F�̃}�b�v
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

		const Scene& scene = Director::getInstance()->getScene();


		const AmbientLight* ambientLight = scene.getAmbientLight();
		Logger::logAssert(ambientLight != nullptr, "�V�[���ɃA���r�G���g���C�g���Ȃ��B");
		// �A���r�G���g���C�g�J���[�̃}�b�v
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


		ID3D11ShaderResourceView* depthTextureResourceView = nullptr;

		const DirectionalLight* directionalLight = scene.getDirectionalLight();
		if (directionalLight != nullptr)
		{
			// TODO:�Ƃ肠�����e����DirectionalLight�݂̂�z��
			// ���̕����Ɍ����ăV���h�E�}�b�v�����J�����������Ă���ƍl���A�J�������猩�����f�����W�n�ɂ���
			if (directionalLight->hasShadowMap())
			{
				result = direct3dContext->Map(
					_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				Mat4 lightViewMatrix = directionalLight->getShadowMapData().viewMatrix;
				lightViewMatrix.transpose();
				CopyMemory(mappedResource.pData, &lightViewMatrix.m, sizeof(lightViewMatrix));
				direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX), 0);

				result = direct3dContext->Map(
					_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX),
					0,
					D3D11_MAP_WRITE_DISCARD,
					0,
					&mappedResource
				);
				Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
				Mat4 lightProjectionMatrix = directionalLight->getShadowMapData().projectionMatrix;
				lightProjectionMatrix = Mat4::CHIRARITY_CONVERTER * lightProjectionMatrix; // ����n�ϊ��s��̓v���W�F�N�V�����s��ɍŏ����炩���Ă���
				lightProjectionMatrix.transpose();
				CopyMemory(mappedResource.pData, &lightProjectionMatrix.m, sizeof(lightProjectionMatrix));
				direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX), 0);

				depthTextureResourceView = directionalLight->getShadowMapData().depthTexture->getShaderResourceView();
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
		}


		// TODO:�|�C���g���C�g�̉e�t���������˂�
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

			CopyMemory(&pointLightConstBufData[i], pointLight->getConstantBufferDataPointer(), sizeof(PointLight::ConstantBufferData));
		}

		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER), 0);


		// TODO:�X�|�b�g���C�g�̉e�t���������˂�
		// �X�|�b�g���C�g�̈ʒu�������W�̋t���̃}�b�v
		result = direct3dContext->Map(
			_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);

		SpotLight::ConstantBufferData* spotLightConstBufData = static_cast<SpotLight::ConstantBufferData*>(mappedResource.pData); ZeroMemory(spotLightConstBufData, sizeof(SpotLight::ConstantBufferData) * SpotLight::MAX_NUM);

		size_t numSpotLight = scene.getNumSpotLight();
		for (size_t i = 0; i < numSpotLight; i++)
		{
			const SpotLight* spotLight = scene.getSpotLight(i);

			CopyMemory(&spotLightConstBufData[i], spotLight->getConstantBufferDataPointer(), sizeof(SpotLight::ConstantBufferData));
		}

		direct3dContext->Unmap(_d3dProgramForForwardRendering.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER), 0);


		if (_isC3b)
		{
			// �W���C���g�}�g���b�N�X�p���b�g�̃}�b�v
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

		UINT strides[1];
		if (_isObj)
		{
			strides[0] = sizeof(Position3DNormalTextureCoordinates);
		}
		else if (_isC3b)
		{
			strides[0] = _perVertexByteSize;
		}

		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForForwardRendering.getVertexBuffers().size(), _d3dProgramForForwardRendering.getVertexBuffers().data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForForwardRendering.getIndexBuffer(), DXGI_FORMAT_R16_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForForwardRendering.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		_d3dProgramForForwardRendering.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForForwardRendering.setConstantBuffersToDirect3DContext(direct3dContext);

		if (depthTextureResourceView == nullptr) // �V���h�E�}�b�v������ĂȂ��Ƃ�
		{
			ID3D11ShaderResourceView* resourceView[1] = { _texture->getShaderResourceView() };
			direct3dContext->PSSetShaderResources(0, 1, resourceView);
		}
		else // �V���h�E�}�b�v��������Ƃ�
		{
			ID3D11ShaderResourceView* resourceViews[2] = {_texture->getShaderResourceView(), depthTextureResourceView};
			direct3dContext->PSSetShaderResources(0, 2, resourceViews);
		}

		ID3D11SamplerState* samplerState[2] = { Director::getRenderer().getLinearSamplerState(), Director::getRenderer().getPCFSamplerState() };
		direct3dContext->PSSetSamplers(0, 2, samplerState);

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgramForForwardRendering.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->DrawIndexed(_indices.size(), 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		// cocos2d-x��TriangleCommand���s���Ă�`������ȁB�B�e�N�X�`���o�C���h��Texture2D�ł���Ă�̂ɑ��v���H
		glUseProgram(_glProgram.getShaderProgram());
		GLProgram::checkGLError();

		glUniform3f(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		GLProgram::checkGLError();

		// �s��̐ݒ�
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		GLProgram::checkGLError();

		Mat4 normalMatrix = Mat4::createNormalMatrix(getModelMatrix());
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_NORMAL_MATRIX), 1, GL_FALSE, (GLfloat*)&normalMatrix.m);

		// ���C�g�̐ݒ�
		// TODO:����A���C�g�͊e��ނ��ƂɈ�������������ĂȂ��B�Ō�̂�ŏ㏑���B
		for (Light* light : Director::getLight())
		{
			const Color3B& lightColor = light->getColor();
			float intensity = light->getIntensity();

			switch (light->getLightType())
			{
			case LightType::AMBIENT:
				glUniform3f(_glProgram.getUniformLocation("u_ambientLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
				GLProgram::checkGLError();
				break;
			case LightType::DIRECTION: {
				glUniform3f(_glProgram.getUniformLocation("u_directionalLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
				GLProgram::checkGLError();

				DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
				Vec3 direction = dirLight->getDirection();
				direction.normalize();
				glUniform3fv(_glProgram.getUniformLocation("u_directionalLightDirection"), 1, (GLfloat*)&direction);
				GLProgram::checkGLError();

				// TODO:�Ƃ肠�����e����DirectionalLight�݂̂�z��
				// ���̕����Ɍ����ăV���h�E�}�b�v�����J�����������Ă���ƍl���A�J�������猩�����f�����W�n�ɂ���
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

					static Mat4 depthBiasMatrix = Mat4::createScale(Vec3(0.5f, 0.5f, 0.5f)) * Mat4::createTranslation(Vec3(1.0f, 1.0f, 1.0f));

					glUniformMatrix4fv(
						_glProgram.getUniformLocation("u_depthBiasMatrix"),
						1,
						GL_FALSE,
						(GLfloat*)depthBiasMatrix.m
					);
					// TODO:Vec3��Mat4�ɓ��ɂ���-���Z�q���Ȃ���

					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, dirLight->getShadowMapData().getDepthTexture()->getTextureId());
					glUniform1i(_glProgram.getUniformLocation("u_shadowTexture"), 1);
					glActiveTexture(GL_TEXTURE0);
				}
			}
				break;
			case LightType::POINT: {
				glUniform3f(_glProgram.getUniformLocation("u_pointLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
				GLProgram::checkGLError();

				glUniform3fv(_glProgram.getUniformLocation("u_pointLightPosition"), 1, (GLfloat*)&light->getPosition()); // ���C�g�ɂ��Ă̓��[�J�����W�łȂ����[���h���W�ł���O��
				GLProgram::checkGLError();

				PointLight* pointLight = static_cast<PointLight*>(light);
				glUniform1f(_glProgram.getUniformLocation("u_pointLightRangeInverse"), 1.0f / pointLight->getRange());
				GLProgram::checkGLError();
			}
				break;
			case LightType::SPOT: {
				glUniform3f(_glProgram.getUniformLocation("u_spotLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
				GLProgram::checkGLError();

				glUniform3fv(_glProgram.getUniformLocation("u_spotLightPosition"), 1, (GLfloat*)&light->getPosition());
				GLProgram::checkGLError();

				SpotLight* spotLight = static_cast<SpotLight*>(light);
				Vec3 direction = spotLight->getDirection();
				direction.normalize();
				glUniform3fv(_glProgram.getUniformLocation("u_spotLightDirection"), 1, (GLfloat*)&direction);
				GLProgram::checkGLError();

				glUniform1f(_glProgram.getUniformLocation("u_spotLightRangeInverse"), 1.0f / spotLight->getRange());
				GLProgram::checkGLError();

				glUniform1f(_glProgram.getUniformLocation("u_spotLightInnerAngleCos"), spotLight->getInnerAngleCos());
				GLProgram::checkGLError();

				glUniform1f(_glProgram.getUniformLocation("u_spotLightOuterAngleCos"), spotLight->getOuterAngleCos());
				GLProgram::checkGLError();
			}
			default:
				break;
			}
		}

		// ���_�����̐ݒ�
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
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].position);
			GLProgram::checkGLError();
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].normal);
			GLProgram::checkGLError();
			glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DNormalTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);
			GLProgram::checkGLError();
		}
		else if (_isC3b)
		{
			// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
			C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
			for (size_t i = 0, offset = 0; i < meshData->numAttribute; ++i)
			{
				const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				GLProgram::checkGLError();
				offset += attrib.size;
			}
		}

		// �X�L�j���O�̃}�g���b�N�X�p���b�g�̐ݒ�
		if (_isC3b) {
			Logger::logAssert(_matrixPalette.size() > 0, "�}�g���b�N�X�p���b�g��0�łȂ��O��");
			glUniformMatrix4fv(_glProgram.getUniformLocation("u_matrixPalette"), _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette.data()));
			GLProgram::checkGLError();
		}

		// TODO:monguri:����
		if (_isC3b) {
			glUniform3fv(_glProgram.getUniformLocation("u_cameraPosition"), 1, (GLfloat*)&Director::getCamera().getPosition());
			GLProgram::checkGLError();

			glUniform3fv(_glProgram.getUniformLocation("u_materialAmbient"), 1, (GLfloat*)&_ambient);
			GLProgram::checkGLError();

			glUniform3fv(_glProgram.getUniformLocation("u_materialDiffuse"), 1, (GLfloat*)&_diffuse);
			GLProgram::checkGLError();

			glUniform3fv(_glProgram.getUniformLocation("u_materialSpecular"), 1, (GLfloat*)&_specular);
			GLProgram::checkGLError();

			glUniform1f(_glProgram.getUniformLocation("u_materialShininess"), _shininess);
			GLProgram::checkGLError();

			//glUniform3fv(_glProgram.uniformMaterialEmissive, 1, (GLfloat*)&_emissive);
			//GLProgram::checkGLError();

			//glUniform1f(_glProgram.uniformMaterialOpacity, 1, (GLfloat*)&_emissive);
			//GLProgram::checkGLError();
		}

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		GLProgram::checkGLError();

		glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, _indices.data());
		GLProgram::checkGLError();
		glBindTexture(GL_TEXTURE_2D, 0);
#endif
	});

	Director::getRenderer().addCommand(&_renderForwardCommand);
}

} // namespace mgrrenderer
