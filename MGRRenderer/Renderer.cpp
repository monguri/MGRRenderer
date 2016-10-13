#include "Renderer.h"
#include "Director.h"
#include "RenderCommand.h"
#include "GroupBeginRenderCommand.h"
#include "Logger.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DTexture.h"
#include "Light.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "GLFrameBuffer.h"
#endif

namespace mgrrenderer
{

static const size_t DEFAULT_RENDER_QUEUE_GROUP_INDEX = 0;

Renderer::Renderer() :
#if defined(MGRRENDERER_USE_DIRECT3D)
_gBufferDepthStencil(nullptr),
_gBufferColorSpecularIntensity(nullptr),
_gBufferNormal(nullptr),
_gBufferSpecularPower(nullptr)
#elif defined(MGRRENDERER_USE_OPENGL)
_gBufferFrameBuffer(nullptr)
#endif
{
	_groupIndexStack.push(DEFAULT_RENDER_QUEUE_GROUP_INDEX);

	std::vector<RenderCommand*> defaultRenderQueue;
	_queueGroup.push_back(defaultRenderQueue);
}

Renderer::~Renderer()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	if (_gBufferSpecularPower != nullptr)
	{
		delete _gBufferSpecularPower;
		_gBufferSpecularPower = nullptr;
	}

	if (_gBufferNormal != nullptr)
	{
		delete _gBufferNormal;
		_gBufferNormal = nullptr;
	}

	if (_gBufferColorSpecularIntensity != nullptr)
	{
		delete _gBufferColorSpecularIntensity;
		_gBufferColorSpecularIntensity = nullptr;
	}

	if (_gBufferDepthStencil != nullptr)
	{
		delete _gBufferDepthStencil;
		_gBufferDepthStencil = nullptr;
	}
#elif defined(MGRRENDERER_USE_OPENGL)
	if (_gBufferFrameBuffer != nullptr)
	{
		delete _gBufferFrameBuffer;
		_gBufferFrameBuffer = nullptr;
	}
#endif
}

void Renderer::initView(const Size& windowSize)
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	// �r���[�|�[�g�̏���
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
	direct3dContext->RSSetViewports(1, Director::getInstance()->getDirect3dViewport());

	// G�o�b�t�@�̏���
	_gBufferDepthStencil = new D3DTexture();
	_gBufferDepthStencil->initDepthStencilTexture(windowSize);

	_gBufferColorSpecularIntensity = new D3DTexture();
	_gBufferColorSpecularIntensity->initRenderTexture(windowSize, DXGI_FORMAT_R8G8B8A8_UNORM);

	_gBufferNormal = new D3DTexture();
	_gBufferNormal->initRenderTexture(windowSize, DXGI_FORMAT_R11G11B10_FLOAT);

	_gBufferSpecularPower = new D3DTexture();
	_gBufferSpecularPower->initRenderTexture(windowSize, DXGI_FORMAT_R8G8B8A8_UNORM);

	//
	// �f�B�t�@�[�h�����_�����O�̏���
	//

	_d3dProgram.initWithShaderFile("DeferredLighting.hlsl", true, "VS", "", "PS");

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
	ID3D11Device* direct3dDevice = Director::getInstance()->getDirect3dDevice();
	// View�s��p
	constantBuffer = nullptr;
	HRESULT result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX, constantBuffer);

	// Projection�s��p
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX, constantBuffer);

	// �A���r�G���g���C�g�J���[
	constantBufferDesc.ByteWidth = sizeof(AmbientLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_AMBIENT_LIGHT_PARAMETER, constantBuffer);

	// �f�B���N�V���i���g���C�gView�s��p
	constantBufferDesc.ByteWidth = sizeof(Mat4);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_VIEW_MATRIX, constantBuffer);

	// �f�B���N�V���i���g���C�gProjection�s��p
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PROJECTION_MATRIX, constantBuffer);

	// �f�B���N�V���i���g���C�g�f�v�X�o�C�A�X�s��p
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_DEPTH_BIAS_MATRIX, constantBuffer);

	// �f�B���N�V���i���g���C�g�p�����[�^�[
	constantBufferDesc.ByteWidth = sizeof(DirectionalLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_PARAMETER, constantBuffer);

	// �|�C���g���C�g�p�����[�^�[
	constantBufferDesc.ByteWidth = sizeof(PointLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_POINT_LIGHT_PARAMETER, constantBuffer);

	// �X�|�b�g���C�g�p�����[�^�[
	constantBufferDesc.ByteWidth = sizeof(SpotLight::ConstantBufferData);
	constantBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addConstantBuffer(D3DProgram::CONSTANT_BUFFER_SPOT_LIGHT_PARAMETER, constantBuffer);

	_quadrangle.bottomLeft.position = Vec2(-1.0f, -1.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.bottomRight.position = Vec2(1.0, -1.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0, 1.0f);
	_quadrangle.topLeft.position = Vec2(-1.0f, 1.0f);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.topRight.position = Vec2(1.0, 1.0);
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
	ID3D11Buffer* vertexBuffer = nullptr;
	result = direct3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubData, &vertexBuffer);
	if (FAILED(result))
	{
		Logger::logAssert(false, "CreateBuffer failed. result=%d", result);
		return;
	}
	_d3dProgram.addVertexBuffer(vertexBuffer);

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
		return;
	}
	_d3dProgram.setInputLayout(inputLayout);
#elif defined(MGRRENDERER_USE_OPENGL)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// TODO:�u�����h���K�v�Ȃ������u�����h��ON�ɂ��Ă���
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// OpenGL���ł��r���[�|�[�g�ϊ��̂��߂̃p�����[�^��n��
	glViewport(0, 0, static_cast<GLsizei>(windowSize.width), static_cast<GLsizei>(windowSize.height));
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // �f�t�H���g�̃t���[���o�b�t�@

	_gBufferFrameBuffer = new GLFrameBuffer();
	std::vector<GLenum> drawBuffers;
	drawBuffers.push_back(GL_NONE);
	drawBuffers.push_back(GL_COLOR_ATTACHMENT0);
	drawBuffers.push_back(GL_COLOR_ATTACHMENT1);
	drawBuffers.push_back(GL_COLOR_ATTACHMENT2);
	std::vector<GLenum> pixelFormats;
	pixelFormats.push_back(GL_DEPTH_COMPONENT);
	pixelFormats.push_back(GL_RGBA);
	pixelFormats.push_back(GL_RGBA);
	pixelFormats.push_back(GL_RGBA);
	_gBufferFrameBuffer->initWithTextureParams(drawBuffers, pixelFormats, false, windowSize);
#endif
}

void Renderer::addCommand(RenderCommand* command)
{
	switch (command->getType())
	{
	case RenderCommand::Type::GROUP_BEGIN:
		{
			// �X�^�b�N�̃g�b�v�̃C���f�b�N�X�̃L���[�ɒǉ�
			_queueGroup[_groupIndexStack.top()].push_back(command);

			// �X�^�b�N�Ƀv�b�V��
			size_t groupIndex = _queueGroup.size();
			GroupBeginRenderCommand* groupCommand = static_cast<GroupBeginRenderCommand*>(command);
			groupCommand->setGroupIndex(groupIndex);
			_groupIndexStack.push(groupIndex);

			std::vector<RenderCommand*> newRenderQueue;
			_queueGroup.push_back(newRenderQueue);
		}
		break;
	case RenderCommand::Type::GROUP_END:
		// addCommand�ł́A���ݑ��쒆�̃O���[�vID��m�邽�߂ɃX�^�b�N����͍폜���邪�A�L���[�O���[�v����͍폜���Ȃ��B
		_groupIndexStack.pop();
		_queueGroup[_groupIndexStack.top()].push_back(command);
		break;
	case RenderCommand::Type::CUSTOM:
		// �X�^�b�N�̃g�b�v�̃L���[�ɒǉ�
		_queueGroup[_groupIndexStack.top()].push_back(command);
		break;
	default:
		Logger::logAssert(false, "�Ή����Ă��Ȃ��R�}���h�^�C�v�����͂��ꂽ�B");
		break;
	}
}

void Renderer::render()
{
	visitRenderQueue(_queueGroup[DEFAULT_RENDER_QUEUE_GROUP_INDEX]);

	Logger::logAssert(_groupIndexStack.size() == 1, "�O���[�v�R�}���h�J�n���ō��ꂽ�C���f�b�N�X�X�^�b�N�͏I�����ŏ�����Ă�͂��B_groupIndexStack.size() == %d", _queueGroup.size());

	// 0�Ԗڂ̃L���[���c���Ă��Ƃ͍폜�B���t���[���̃O���[�v�R�}���h�ł܂��ǉ�����B�폜����̂́A�V�[���̏󋵂ŃO���[�v���͕ς�肤��̂ŁA�������܂܂Ŏc���Ă����Ă����ʂ�����B
	while (_queueGroup.size() > 1)
	{
		_queueGroup.pop_back();
	}

	_queueGroup[DEFAULT_RENDER_QUEUE_GROUP_INDEX].clear();
}

void Renderer::prepareGBufferRendering()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
	direct3dContext->ClearState();

	float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	direct3dContext->ClearRenderTargetView(_gBufferColorSpecularIntensity->getRenderTargetView(), clearColor);
	direct3dContext->ClearRenderTargetView(_gBufferNormal->getRenderTargetView(), clearColor);
	direct3dContext->ClearRenderTargetView(_gBufferSpecularPower->getRenderTargetView(), clearColor);
	direct3dContext->ClearDepthStencilView(_gBufferDepthStencil->getDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	direct3dContext->RSSetViewports(1, Director::getInstance()->getDirect3dViewport());

	ID3D11RenderTargetView* gBuffers[3] = {_gBufferColorSpecularIntensity->getRenderTargetView(), _gBufferNormal->getRenderTargetView(), _gBufferSpecularPower->getRenderTargetView()};
	direct3dContext->OMSetRenderTargets(3, gBuffers, _gBufferDepthStencil->getDepthStencilView());
	direct3dContext->OMSetDepthStencilState(_gBufferDepthStencil->getDepthStencilState(), 1);
#elif defined(MGRRENDERER_USE_OPENGL)
	glBindFramebuffer(GL_FRAMEBUFFER, _gBufferFrameBuffer->getFrameBufferId());

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// G�o�b�t�@�̑傫���͉�ʃT�C�Y�Ɠ����ɂ��Ă���
	glViewport(0, 0, static_cast<GLsizei>(Director::getInstance()->getWindowSize().width), static_cast<GLsizei>(Director::getInstance()->getWindowSize().height));

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND); // G�o�b�t�@�`�撆�͕s���߂���������Ȃ��̂Ńu�����h���Ȃ�
#endif
}

void Renderer::prepareDeferredRendering()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();
	direct3dContext->ClearState();

	float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	direct3dContext->ClearRenderTargetView(Director::getInstance()->getDirect3dRenderTarget(), clearColor);
	direct3dContext->ClearDepthStencilView(Director::getInstance()->getDirect3dDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	direct3dContext->RSSetViewports(1, Director::getInstance()->getDirect3dViewport());
	ID3D11RenderTargetView* renderTarget = Director::getInstance()->getDirect3dRenderTarget(); //TODO: ��x�ϐ��ɓ���Ȃ��ƃR���p�C���G���[���o�Ă��܂���
	direct3dContext->OMSetRenderTargets(1, &renderTarget, Director::getInstance()->getDirect3dDepthStencilView());
	direct3dContext->OMSetDepthStencilState(Director::getInstance()->getDirect3dDepthStencilState(), 1);
#elif defined(MGRRENDERER_USE_OPENGL)
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glViewport(0, 0, static_cast<GLsizei>(Director::getInstance()->getWindowSize().width), static_cast<GLsizei>(Director::getInstance()->getWindowSize().height));
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // �f�t�H���g�t���[���o�b�t�@�ɖ߂�
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void Renderer::renderDeferred()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	ID3D11DeviceContext* direct3dContext = Director::getInstance()->getDirect3dContext();

	// TODO:������ւ񋤒ʉ��������ȁB�B
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// �r���[�s��̋t�s��̃}�b�v
	HRESULT result = direct3dContext->Map(
		_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
	Mat4 viewMatrix = Director::getCamera().getViewMatrix();
	viewMatrix.inverse();
	viewMatrix.transpose(); // Direct3D�ł͓]�u������Ԃœ����
	CopyMemory(mappedResource.pData, &viewMatrix.m, sizeof(viewMatrix));
	direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_VIEW_MATRIX), 0);

	// �v���W�F�N�V�����s��̃}�b�v
	result = direct3dContext->Map(
		_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
	);
	Logger::logAssert(SUCCEEDED(result), "Map failed, result=%d", result);
	Mat4 projectionMatrix = Director::getCamera().getProjectionMatrix();
	projectionMatrix = Mat4::CHIRARITY_CONVERTER * projectionMatrix; // ����n�ϊ��s��̓v���W�F�N�V�����s��ɍŏ����炩���Ă���
	projectionMatrix.transpose();
	CopyMemory(mappedResource.pData, &projectionMatrix.m, sizeof(projectionMatrix));
	direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_PROJECTION_MATRIX), 0);

	ID3D11ShaderResourceView* depthTextureResourceView = nullptr;
	ID3D11SamplerState* depthTextureSamplerState = nullptr;
	// ���C�g�̐ݒ�
	// TODO:����A���C�g�͊e��ނ��ƂɈ�������������ĂȂ��B�Ō�̂�ŏ㏑���B
	for (Light* light : Director::getLight())
	{
		switch (light->getLightType())
		{
		case LightType::AMBIENT:
		{
			// �A���r�G���g���C�g�J���[�̃}�b�v
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

			// TODO:�Ƃ肠�����e����DirectionalLight�݂̂�z��
			// ���̕����Ɍ����ăV���h�E�}�b�v�����J�����������Ă���ƍl���A�J�������猩�����f�����W�n�ɂ���
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
				lightProjectionMatrix = Mat4::CHIRARITY_CONVERTER * lightProjectionMatrix; // ����n�ϊ��s��̓v���W�F�N�V�����s��ɍŏ����炩���Ă���
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
				Mat4 depthBiasMatrix = (Mat4::TEXTURE_COORDINATE_CONVERTER * Mat4::createScale(Vec3(0.5f, 0.5f, 1.0f)) * Mat4::createTranslation(Vec3(1.0f, -1.0f, 0.0f))).transpose(); //TODO: Mat4���Q�ƌ^�ɂ���ƒl�����������Ȃ��Ă��܂�
				CopyMemory(mappedResource.pData, &depthBiasMatrix.m, sizeof(depthBiasMatrix));
				direct3dContext->Unmap(_d3dProgram.getConstantBuffer(D3DProgram::CONSTANT_BUFFER_DIRECTIONAL_LIGHT_DEPTH_BIAS_MATRIX), 0);

				depthTextureResourceView = dirLight->getShadowMapData().depthTexture->getShaderResourceView();
				depthTextureSamplerState = dirLight->getShadowMapData().depthTexture->getSamplerState();
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
			// �X�|�b�g���C�g�̈ʒu�������W�̋t���̃}�b�v
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

	UINT strides[1] = {sizeof(_quadrangle.topLeft)};
	UINT offsets[1] = {0};
	direct3dContext->IASetVertexBuffers(0, _d3dProgram.getVertexBuffers().size(), _d3dProgram.getVertexBuffers().data(), strides, offsets);
	direct3dContext->IASetInputLayout(_d3dProgram.getInputLayout());
	//direct3dContext->IASetInputLayout(nullptr);
	direct3dContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	_d3dProgram.setShadersToDirect3DContext(direct3dContext);
	_d3dProgram.setConstantBuffersToDirect3DContext(direct3dContext);

	direct3dContext->RSSetState(_d3dProgram.getRasterizeState());

	ID3D11ShaderResourceView* gBufferShaderResourceViews[4] = {
		getGBufferDepthStencil()->getShaderResourceView(),
		getGBufferColorSpecularIntensity()->getShaderResourceView(),
		getGBufferNormal()->getShaderResourceView(),
		getGBufferSpecularPower()->getShaderResourceView(),
	};
	direct3dContext->PSSetShaderResources(0, 4, gBufferShaderResourceViews);

	// TODO:�T���v���̓e�N�X�`�����Ƃɍ��K�v�͂Ȃ�
	ID3D11SamplerState* samplerState = getGBufferDepthStencil()->getSamplerState(); //TODO:�^�ϊ������܂������Ȃ��̂ň�x�ϐ��ɑ�����Ă���
	direct3dContext->PSSetSamplers(0, 1, &samplerState);

	FLOAT blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	direct3dContext->OMSetBlendState(_d3dProgram.getBlendState(), blendFactor, 0xffffffff);

	direct3dContext->Draw(4, 0);
#endif
}

void Renderer::prepareFowardRendering()
{
	// ������ɂ��邱�Ƃ��Ȃ��B��������prepare���Ȃ��ƑΏ̂łȂ����ߒ�`����
}

void Renderer::prepareFowardRendering2D()
{
#if defined(MGRRENDERER_USE_DIRECT3D)
	Director::getInstance()->getDirect3dContext()->OMSetDepthStencilState(Director::getInstance()->getDirect3dDepthStencilState2D(), 1);
#elif defined(MGRRENDERER_USE_OPENGL)
	glDisable(GL_DEPTH_TEST);
#endif
}

void Renderer::visitRenderQueue(const std::vector<RenderCommand*> queue)
{
	for (RenderCommand* command : queue)
	{
		// TODO:for-each�łȂ������ƃR���N�V�����ɑ΂���p�C�v�݂����ȃ��\�b�h�g��������
		executeRenderCommand(command);
	}
}

void Renderer::executeRenderCommand(RenderCommand* command)
{
	switch (command->getType())
	{
	case RenderCommand::Type::GROUP_BEGIN:
		{
			GroupBeginRenderCommand* groupCommand = static_cast<GroupBeginRenderCommand*>(command);
			groupCommand->execute();
			visitRenderQueue(_queueGroup[groupCommand->getGroupIndex()]);
		}
		break;
	case RenderCommand::Type::GROUP_END:
		{
			GroupBeginRenderCommand* groupCommand = static_cast<GroupBeginRenderCommand*>(command);
			groupCommand->execute();
		}
		break;
	case RenderCommand::Type::CUSTOM:
		command->execute();
		break;
	default:
		Logger::logAssert(false, "�Ή����Ă��Ȃ��R�}���h�^�C�v�����͂��ꂽ�B");
		break;
	}
}

} // namespace mgrrenderer
