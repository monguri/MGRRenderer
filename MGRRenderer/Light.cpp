#include "Light.h"
#include "Director.h"

namespace mgrrenderer
{

Light::Light() : _intensity(1.0f)
{
}

AmbientLight::AmbientLight(const Color3B& color)
{
	setColor(color);
}

DirectionalLight::DirectionalLight(const Vec3& direction, const Color3B& color) : _direction(direction), _hasShadowMap(false)
{
	setColor(color);
}

void DirectionalLight::prepareShadowMap(const Vec3& targetPosition, float cameraDistanceFromTaret, const Size& size)
{
	_hasShadowMap = true;

	_shadowMapData.viewMatrix = Mat4::createLookAt(
		getDirection() * (-1) * cameraDistanceFromTaret + targetPosition, // �߂�����ƃJ�����ɓ���Ȃ��̂œK�x�ɗ���
		targetPosition,
		Vec3(0.0f, 1.0f, 0.0f) // �Ƃ肠����y��������ɂ��ČŒ�
	);

	// �f�v�X�e�N�X�`���쐬
	glGenTextures(1, &_shadowMapData.textureId);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	Logger::logAssert(_shadowMapData.textureId != 0, "�f�v�X�e�N�X�`���������s");

	glBindTexture(GL_TEXTURE_2D, _shadowMapData.textureId);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//TODO: �K���Ƀf�v�X�e�N�X�`���𑜓x�̓E�C���h�E�T�C�Y�Ɠ����ɂ��Ă���
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.width, size.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glBindTexture(GL_TEXTURE_2D, 0);

	// �f�v�X�e�N�X�`���ɕ`�悷�邽�߂̃t���[���o�b�t�@�쐬
	glGenFramebuffers(1, &_shadowMapData.frameBufferId);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	Logger::logAssert(_shadowMapData.frameBufferId != 0, "�t���[���o�b�t�@�������s");

	glBindFramebuffer(GL_FRAMEBUFFER, _shadowMapData.frameBufferId);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowMapData.textureId, 0);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	Logger::logAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "�f�v�X�V���h�E�p�̃t���[���o�b�t�@���������ĂȂ�");

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // �f�t�H���g�̃t���[���o�b�t�@�ɖ߂�
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
}

PointLight::PointLight(const Vec3& position, const Color3B& color, float range) : _range(range)
{
	setPosition(position);
	setColor(color);
}

SpotLight::SpotLight(const Vec3& position, const Vec3& direction, const Color3B& color, float range, float innerAngle, float outerAngle) :
_direction(direction),
_range(range),
_innerAngleCos(cosf(innerAngle)),
_outerAngleCos(cosf(outerAngle))
{
	setPosition(position);
	setColor(color);
}

} // namespace mgrrenderer
