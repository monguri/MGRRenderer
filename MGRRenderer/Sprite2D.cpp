#include "Sprite2D.h"
#include "FileUtility.h"
#include "Image.h"
#include "Director.h"
#include "Shaders.h"
#include "TextureUtility.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DTexture.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "GLTexture.h"
#endif

namespace mgrrenderer
{

Sprite2D::Sprite2D() :
_texture(nullptr)
{
}

Sprite2D::~Sprite2D()
{
#if defined(MGRRENDERER_USE_OPENGL)
	glBindTexture(GL_TEXTURE_2D, 0);
#endif

	if (_texture)
	{
		delete _texture;
		_texture = nullptr;
	}
}

bool Sprite2D::init(const std::string& filePath)
{
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
	const Size& contentSize = texture->getContentSize();

	// TODO:initWithTexture�Ƌ��ʉ����ׂ�
	_quadrangle.bottomLeft.position = Vec2(0.0f, 0.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.bottomRight.position = Vec2(contentSize.width, 0.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0f, 1.0f);
	_quadrangle.topLeft.position = Vec2(0.0f, contentSize.height);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.topRight.position = Vec2(contentSize.width, contentSize.height);
	_quadrangle.topRight.textureCoordinate = Vec2(1.0f, 0.0f);

#if defined(MGRRENDERER_USE_DIRECT3D)
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
	ID3D11Device* direct3dDevice = Director::getInstance()->getDirect3dDevice();
	ID3D11Buffer* vertexBuffer = nullptr;
	HRESULT result = direct3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubData, &vertexBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.setVertexBuffer(vertexBuffer);

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
	_d3dProgram.setIndexBuffer(indexBuffer);

	bool depthEnable = false;
	_d3dProgram.initWithShaderFile("PositionTextureMultiplyColor.hlsl", depthEnable);

	// ���̓��C�A�E�g�I�u�W�F�N�g�̍쐬
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
	_d3dProgram.addConstantBuffer(constantBuffer);

	// View�s��p
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(constantBuffer);

	// Projection�s��p
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(constantBuffer);

	constantBufferDesc.ByteWidth = sizeof(Color4F); // getColor()��Color3B�ɂ����12�o�C�g���E�Ȃ̂�16�o�C�g���E�̂��߂Ƀp�f�B���O�f�[�^�����˂΂Ȃ�Ȃ�
	// ��Z�F
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return false;
	}
	_d3dProgram.addConstantBuffer(constantBuffer);
#elif defined(MGRRENDERER_USE_OPENGL)
	_glProgram.initWithShaderString(shader::VERTEX_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR, shader::FRAGMENT_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR);
#endif

	return true;
}

#if defined(MGRRENDERER_USE_OPENGL)
bool Sprite2D::initWithTexture(GLuint textureId, const Size& contentSize, TextureUtility::PixelFormat format)
{
	// TODO:���߂�new����K�v���邩�H
	_texture = new GLTexture(); // Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
	_texture->initWithTexture(textureId, contentSize, format);

	//TODO: ��Z���钸�_�J���[�ɂ͑Ή����Ȃ�

	_glProgram.initWithShaderString(shader::VERTEX_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR, shader::FRAGMENT_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR);

	_quadrangle.bottomLeft.position = Vec2(0.0f, 0.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.bottomRight.position = Vec2(contentSize.width, 0.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0f, 1.0f);
	_quadrangle.topLeft.position = Vec2(0.0f, contentSize.height);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.topRight.position = Vec2(contentSize.width, contentSize.height);
	_quadrangle.topRight.textureCoordinate = Vec2(1.0f, 0.0f);

	return true;
}
#endif

void Sprite2D::renderWithShadowMap()
{
	_renderCommand.init([=]
	{
#if defined(MGRRENDERER_USE_DIRECT3D)
		ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();

		// TODO:������ւ񋤒ʉ��������ȁB�B
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// ���f���s��̃}�b�v
		HRESULT result = direct3dContext->Map(
			_d3dProgram.getConstantBuffers()[0],
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 modelMatrix = getModelMatrix();
		modelMatrix.transpose();
		CopyMemory(mappedResource.pData, &modelMatrix.m, sizeof(modelMatrix));
		direct3dContext->Unmap(_d3dProgram.getConstantBuffers()[0], 0);

		// �r���[�s��̃}�b�v
		result = direct3dContext->Map(
			_d3dProgram.getConstantBuffers()[1],
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 viewMatrix = Director::getCameraFor2D().getViewMatrix();
		viewMatrix.transpose(); // Direct3D�ł͓]�u������Ԃœ����
		CopyMemory(mappedResource.pData, &viewMatrix.m, sizeof(viewMatrix));
		direct3dContext->Unmap(_d3dProgram.getConstantBuffers()[1], 0);

		// �v���W�F�N�V�����s��̃}�b�v
		result = direct3dContext->Map(
			_d3dProgram.getConstantBuffers()[2],
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		Mat4 projectionMatrix = Director::getCameraFor2D().getProjectionMatrix();
		projectionMatrix = Mat4::CHIRARITY_CONVERTER * projectionMatrix; // ����n�ϊ��s��̓v���W�F�N�V�����s��ɍŏ����炩���Ă���
		projectionMatrix.transpose();
		CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
		direct3dContext->Unmap(_d3dProgram.getConstantBuffers()[2], 0);

		// ��Z�F�̃}�b�v
		result = direct3dContext->Map(
			_d3dProgram.getConstantBuffers()[3],
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
		const Color4F& multiplyColor = Color4F(Color4B(getColor().r, getColor().g, getColor().b, 255));
		CopyMemory(mappedResource.pData, &multiplyColor , sizeof(multiplyColor));
		direct3dContext->Unmap(_d3dProgram.getConstantBuffers()[3], 0);

		UINT strides[1] = {sizeof(_quadrangle.topLeft)};
		UINT offsets[1] = {0};
		ID3D11Buffer* vertexBuffer = _d3dProgram.getVertexBuffer();
		direct3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, strides, offsets);
		direct3dContext->IASetIndexBuffer(_d3dProgram.getIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
		direct3dContext->IASetInputLayout(_d3dProgram.getInputLayout());
		direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		direct3dContext->VSSetShader(_d3dProgram.getVertexShader(), nullptr, 0);
		direct3dContext->VSSetConstantBuffers(0, 4, _d3dProgram.getConstantBuffers().data());

		direct3dContext->GSSetShader(_d3dProgram.getGeometryShader(), nullptr, 0);
		direct3dContext->GSSetConstantBuffers(0, 4, _d3dProgram.getConstantBuffers().data());

		direct3dContext->RSSetState(_d3dProgram.getRasterizeState());

		direct3dContext->PSSetShader(_d3dProgram.getPixelShader(), nullptr, 0);
		direct3dContext->PSSetConstantBuffers(0, 4, _d3dProgram.getConstantBuffers().data());
		ID3D11ShaderResourceView* resourceView = _texture->getShaderResourceView(); //TODO:�^�ϊ������܂������Ȃ��̂ň�x�ϐ��ɑ�����Ă���
		direct3dContext->PSSetShaderResources(0, 1, &resourceView);
		ID3D11SamplerState* samplerState = _texture->getSamplerState();
		direct3dContext->PSSetSamplers(0, 1, &samplerState); //TODO:�^�ϊ������܂������Ȃ��̂ň�x�ϐ��ɑ�����Ă���

		FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		direct3dContext->OMSetBlendState(_d3dProgram.getBlendState(), blendFactor, 0xffffffff);

		direct3dContext->OMSetDepthStencilState(_d3dProgram.getDepthStancilState(), 0);

		direct3dContext->DrawIndexed(4, 0, 0);
#elif defined(MGRRENDERER_USE_OPENGL)
		glDisable(GL_DEPTH_TEST);

		// cocos2d-x��TriangleCommand���s���Ă�`������ȁB�B�e�N�X�`���o�C���h��Texture2D�ł���Ă�̂ɑ��v���H
		glUseProgram(_glProgram.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniform3f(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.position);
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.textureCoordinate);

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
