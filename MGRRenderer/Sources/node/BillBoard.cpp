#include "BillBoard.h"
#include "renderer/Director.h"
#include "renderer/GLProgram.h"
#include "renderer/Image.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "renderer/D3DTexture.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "renderer/GLTexture.h"
#include "renderer/Shaders.h"
#endif


namespace mgrrenderer
{

BillBoard::BillBoard() : _mode(Mode::VIEW_PLANE_ORIENTED)
{
}

bool BillBoard::init(const std::string& filePath, Mode mode)
{
	_mode = mode;

	// Texture�����[�h���Apng��jpeg�𐶃f�[�^�ɂ��AOpenGL�ɂ�����d�g�݂����˂΁B�BSprite�̃\�[�X���������Ƃ����B
	Image image; // Image��CPU���̃��������g���Ă���̂ł��̃X�R�[�v�ŉ������Ă��悢���̂�����X�^�b�N�Ɏ��
	image.initWithFilePath(filePath);

	// Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
#if defined(MGRRENDERER_USE_DIRECT3D)
	_texture = new D3DTexture();
#elif defined(MGRRENDERER_USE_OPENGL)
	_texture = new GLTexture();
#endif

	Texture* texture = _texture;
	texture->initWithImage(image); // TODO:�Ȃ����ÖقɌp�����N���X�̃��\�b�h���ĂׂȂ�

#if defined(MGRRENDERER_USE_DIRECT3D)
	const SizeUint& contentSize = texture->getContentSize();
	_quadrangle.bottomLeft.position = Vec2(0.0f, 0.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.bottomRight.position = Vec2((float)contentSize.width, 0.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0f, 1.0f);
	_quadrangle.topLeft.position = Vec2(0.0f, (float)contentSize.height);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.topRight.position = Vec2((float)contentSize.width, (float)contentSize.height);
	_quadrangle.topRight.textureCoordinate = Vec2(1.0f, 0.0f);

	// ���_�o�b�t�@�̒�`
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(_quadrangle);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// ���_�o�b�t�@�̃T�u���\�[�X�̒�`
	D3D11_SUBRESOURCE_DATA vertexBufferSubData;
	vertexBufferSubData.pSysMem = &_quadrangle;
	vertexBufferSubData.SysMemPitch = 0;
	vertexBufferSubData.SysMemSlicePitch = 0;

	// ���_�o�b�t�@�̃T�u���\�[�X�̍쐬
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
	_d3dProgramForGBuffer.addVertexBuffers(oneMeshVBs);

	// �C���f�b�N�X�o�b�t�@�p�̔z��̗p�ӁB�f���ɏ����ɔԍ��t������
	std::vector<unsigned int> indexArray;
	indexArray.resize(4);
	for (unsigned int i = 0; i < 4; i++)
	{
		indexArray[i] = i;
	}

	// �C���f�b�N�X�o�b�t�@�̒�`
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(UINT) * 4;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// �C���f�b�N�X�o�b�t�@�̃T�u���\�[�X�̒�`
	D3D11_SUBRESOURCE_DATA indexBufferSubData;
	indexBufferSubData.pSysMem = indexArray.data();
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

	std::vector<ID3D11Buffer*> indexBufferList;
	indexBufferList.push_back(indexBuffer);

	_d3dProgramForForwardRendering.addIndexBuffers(indexBufferList);
	_d3dProgramForGBuffer.addIndexBuffers(indexBufferList);

	_d3dProgramForForwardRendering.initWithShaderFile("Resources/shader/PositionTextureMultiplyColor.hlsl", true, "VS", "", "PS");
	_d3dProgramForGBuffer.initWithShaderFile("Resources/shader/PositionTextureMultiplyColor.hlsl", true, "VS", "", "PS_GBUFFER");

	// ���̓��C�A�E�g�I�u�W�F�N�g�̍쐬
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
	_d3dProgramForGBuffer.setInputLayout(inputLayout);

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
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MODEL_MATRIX, constantBuffer);

	// View�s��p
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);

	// Projection�s��p
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);

	// ��Z�F
	constantBuffer = nullptr;
	constantBufferDesc.ByteWidth = sizeof(Color4F); // getColor()��Color3B�ɂ����12�o�C�g���E�Ȃ̂�16�o�C�g���E�̂��߂Ƀp�f�B���O�f�[�^�����˂΂Ȃ�Ȃ�
	result = Director::getRenderer().getDirect3dDevice()->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgramForForwardRendering.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);
	_d3dProgramForGBuffer.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_MULTIPLY_COLOR, constantBuffer);

	return true;
#elif defined(MGRRENDERER_USE_OPENGL)
	// TODO:���͂�Billboard��Sprite2D�p�����Ă�Ӗ����邩�ȁH
	const SizeUint& contentSize = texture->getContentSize();
	_quadrangle.bottomLeft.position = Vec2(0.0f, 0.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.bottomRight.position = Vec2((float)contentSize.width, 0.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0f, 0.0f);
	_quadrangle.topLeft.position = Vec2(0.0f, (float)contentSize.height);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.topRight.position = Vec2((float)contentSize.width, (float)contentSize.height);
	_quadrangle.topRight.textureCoordinate = Vec2(1.0f, 1.0f);

	_glProgramForForwardRendering.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderPositionTexture.glsl", "../MGRRenderer/Resources/shader/FragmentShaderPositionTextureMultiplyColor.glsl");
	_glProgramForGBuffer.initWithShaderFile("../MGRRenderer/Resources/shader/VertexShaderPositionTexture.glsl", "../MGRRenderer/Resources/shader/FragmentShaderPositionTextureMultiplyColorGBuffer.glsl");
	return true;
#endif
}

void BillBoard::prepareRendering()
{
	Node::prepareRendering();

	// TODO:�Ƃ肠����Director�̎��J�����ɂ̂ݑΉ�
	const Camera& camera = Director::getInstance()->getCamera();
	// TODO:����e�q�K�w���Ȃ��̂ŁA���f���ϊ������Ȃ킿���[���h���W�ϊ��B
	const Mat4& cameraWorldMat = camera.getModelMatrix();
	const Mat4& billBoardWorldMat = getModelMatrix();

	Vec3 cameraDir;
	switch (_mode)
	{
	case Mode::VIEW_POINT_ORIENTED:
		cameraDir = Vec3(billBoardWorldMat.m[3][0] - cameraWorldMat.m[3][0], billBoardWorldMat.m[3][1] - cameraWorldMat.m[3][1], billBoardWorldMat.m[3][2] - cameraWorldMat.m[3][2]);
		break;
	case Mode::VIEW_PLANE_ORIENTED:
		cameraDir = (camera.getTargetPosition() - camera.getPosition()); // cocos3.7�͒����_�����_�����ɂȂ��Ă��ĊԈ���Ă���
		break;
	default:
		Logger::logAssert(false, "�z��O��Mode�l�����͂��ꂽ�Bmode=%d", _mode);
		break;
	}

	if (cameraDir.length() < FLOAT_TOLERANCE)
	{
		cameraDir = Vec3(cameraWorldMat.m[2][0], cameraWorldMat.m[2][1], cameraWorldMat.m[2][2]);
	}
	cameraDir.normalize();

	// TODO:���������̐����悭�킩���
	// �J�����̉�]�s����Ƃ���upAxis�ɉ�]�������˂�
	const Vec3& upAxis = Vec3(0, 1, 0);
	//Mat4 rot = camera.getRotationMatrix();
	Vec3 y = camera.getRotationMatrix() * upAxis;
	//Vec3 y = rot * upAxis;
	Vec3 x = cameraDir.cross(y);
	x.normalize();
	y = x.cross(cameraDir);
	y.normalize();

	float xlen = sqrtf(billBoardWorldMat.m[0][0] * billBoardWorldMat.m[0][0] + billBoardWorldMat.m[0][1] * billBoardWorldMat.m[0][1] + billBoardWorldMat.m[0][2] * billBoardWorldMat.m[0][2]);
	float ylen = sqrtf(billBoardWorldMat.m[1][0] * billBoardWorldMat.m[1][0] + billBoardWorldMat.m[1][1] * billBoardWorldMat.m[1][1] + billBoardWorldMat.m[1][2] * billBoardWorldMat.m[1][2]);
	float zlen = sqrtf(billBoardWorldMat.m[2][0] * billBoardWorldMat.m[2][0] + billBoardWorldMat.m[2][1] * billBoardWorldMat.m[2][1] + billBoardWorldMat.m[2][2] * billBoardWorldMat.m[2][2]);

	Mat4 billBoardTransform(
		x.x * xlen, y.x * ylen, -cameraDir.x * zlen,	billBoardWorldMat.m[3][0],
		x.y * xlen,	y.y * ylen,	-cameraDir.y * zlen,	billBoardWorldMat.m[3][1],
		x.z * xlen,	y.z * ylen,	-cameraDir.z * zlen,	billBoardWorldMat.m[3][2],
		0.0f,		0.0f,		0.0f,					1.0f
		);
	setModelMatrix(billBoardTransform);
}

#if defined(MGRRENDERER_DEFERRED_RENDERING)
void BillBoard::renderGBuffer()
{
	// ����A���O��Sprite2D�Ƃ͈قȂ郂�f���s����Z�b�g����̂Ɏg���Ă��邾��
	_renderGBufferCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();

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
		Mat4 projectionMatrix = (Mat4::CHIRARITY_CONVERTER * Director::getCamera().getProjectionMatrix()).transpose(); // ����n�ϊ��s��̓v���W�F�N�V�����s��ɍŏ����炩���Ă���
		CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
		direct3dContext->Unmap(_d3dProgramForGBuffer.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

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

		// ���b�V���͂ЂƂ���
		UINT strides[1] = {sizeof(_quadrangle.topLeft)};
		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForGBuffer.getVertexBuffers(0).size(), _d3dProgramForGBuffer.getVertexBuffers(0).data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForGBuffer.getIndexBuffer()[0][0], DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForGBuffer.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		_d3dProgramForGBuffer.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForGBuffer.setConstantBuffersToDirect3DContext(direct3dContext);

		ID3D11ShaderResourceView* resourceView[1] = { _texture->getShaderResourceView() };
		direct3dContext->PSSetShaderResources(0, 1, resourceView);
		ID3D11SamplerState* samplerState[1] = { Director::getRenderer().getLinearSamplerState() };
		direct3dContext->PSSetSamplers(0, 1, samplerState);

		direct3dContext->DrawIndexed(4, 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgramForGBuffer.getShaderProgram());
		GLProgram::checkGLError();

		glUniform3f(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		GLProgram::checkGLError();

		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgramForGBuffer.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
		GLProgram::checkGLError();

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.position);
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.textureCoordinate);

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif
	});

	Director::getRenderer().addCommand(&_renderGBufferCommand);
}
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)

// Sprite2D�Ƃ̈Ⴂ�͐[�x�e�X�gON�ɂ��Ă邱�Ƃ���
void BillBoard::renderForward()
{
	_renderForwardCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getRenderer().getDirect3dContext();

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

		// ��Z�F�̃}�b�v
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

		// ���b�V���͂ЂƂ���
		UINT strides[1] = {sizeof(_quadrangle.topLeft)};
		UINT offsets[1] = {0};
		direct3dContext->IASetVertexBuffers(0, _d3dProgramForForwardRendering.getVertexBuffers(0).size(), _d3dProgramForForwardRendering.getVertexBuffers(0).data(), strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgramForForwardRendering.getIndexBuffer(0, 0), DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgramForForwardRendering.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		_d3dProgramForForwardRendering.setShadersToDirect3DContext(direct3dContext);
		_d3dProgramForForwardRendering.setConstantBuffersToDirect3DContext(direct3dContext);

		ID3D11ShaderResourceView* resourceView[1] = { _texture->getShaderResourceView() };
		direct3dContext->PSSetShaderResources(0, 1, resourceView);
		ID3D11SamplerState* samplerState[1] = { Director::getRenderer().getLinearSamplerState() };
		direct3dContext->PSSetSamplers(0, 1, samplerState);

		direct3dContext->DrawIndexed(4, 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		// cocos2d-x��TriangleCommand���s���Ă�`������ȁB�B�e�N�X�`���o�C���h��Texture2D�ł���Ă�̂ɑ��v���H
		glUseProgram(_glProgramForForwardRendering.getShaderProgram());
		GLProgram::checkGLError();

		glUniform4f(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f, getOpacity());
		GLProgram::checkGLError();

		glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgramForForwardRendering.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		GLProgram::checkGLError();

		static const Mat4& depthBiasMatrix = Mat4::createScale(Vec3(0.5f, 0.5f, 0.5f)) * Mat4::createTranslation(Vec3(1.0f, 1.0f, 1.0f));
		glUniformMatrix4fv(
			_glProgramForForwardRendering.getUniformLocation("u_depthBiasMatrix"),
			1,
			GL_FALSE,
			(GLfloat*)depthBiasMatrix.m
		);
		GLProgram::checkGLError();

		// �A���r�G���g���C�g
		const Scene& scene = Director::getInstance()->getScene();
		const AmbientLight* ambientLight = scene.getAmbientLight();
		Logger::logAssert(ambientLight != nullptr, "�V�[���ɃA���r�G���g���C�g���Ȃ��B");
		Color3B lightColor = ambientLight->getColor();
		float intensity = ambientLight->getIntensity();
		glUniform3f(_glProgramForForwardRendering.getUniformLocation("u_ambientLightColor"), lightColor.r / 255.0f * intensity, lightColor.g / 255.0f * intensity, lightColor.b / 255.0f * intensity);
		GLProgram::checkGLError();


		// �f�B���N�V���i�����C�g
		const DirectionalLight* directionalLight = scene.getDirectionalLight();
		// ���̕����Ɍ����ăV���h�E�}�b�v�����J�����������Ă���ƍl���A�J�������猩�����f�����W�n�ɂ���
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
				glActiveTexture(GL_TEXTURE0);
			}
		}

		// �|�C���g���C�g
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

				glUniform3fv(glGetUniformLocation(_glProgramForForwardRendering.getShaderProgram(), (std::string("u_pointLightPosition[") + std::to_string(i) + std::string("]")).c_str()), 1, (GLfloat*)&pointLight->getPosition()); // ���C�g�ɂ��Ă̓��[�J�����W�łȂ����[���h���W�ł���O��
				//glUniform3fv(_glProgramForForwardRendering.getUniformLocation(std::string("u_pointLightPosition[") + std::to_string(i) + std::string("]")), 1, (GLfloat*)&pointLight->getPosition()); // ���C�g�ɂ��Ă̓��[�J�����W�łȂ����[���h���W�ł���O��
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

		// �X�|�b�g���C�g
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

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
		GLProgram::checkGLError();

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.position);
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.textureCoordinate);

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif
	});

	Director::getRenderer().addCommand(&_renderForwardCommand);
}

} // namespace mgrrenderer
