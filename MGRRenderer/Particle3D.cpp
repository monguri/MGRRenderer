#include "Particle3D.h"
#include "Director.h"
#include "Texture.h"
#include "Image.h"
// srand�֐��̂���
#include <time.h>

namespace mgrrenderer
{

Particle3D::Particle3D() :
_texture(nullptr),
_elapsedTime(0.0f),
_uniformGravity(-1),
_uniformLifeTime(-1),
_uniformPointSize(-1)
{
}

Particle3D::~Particle3D()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	if (_texture)
	{
		delete _texture;
		_texture = nullptr;
	}
}

bool Particle3D::initWithParameter(const Particle3D::Parameter& parameter)
{
	// ��ł�Particle3D����������|�C���g�X�v���C�g��L���ɂ���B�����L���ɂ��Ȃ���gl_PointCoord���L���ɓ����Ȃ�
	glEnable(GL_POINT_SPRITE);

	_parameter = parameter;

	Logger::logAssert(!parameter.textureFilePath.empty(), "�e�N�X�`���p�X����B");

	Image image; // Image��CPU���̃��������g���Ă���̂ł��̃X�R�[�v�ŉ������Ă��悢���̂�����X�^�b�N�Ɏ��
	image.initWithFilePath(parameter.textureFilePath);

	_texture = new Texture(); // Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
	_texture->initWithImage(image);

	// vec3�Ŗ��܂�̂ŁA�����l��(0,0,0)�Ŗ��܂��Ă���
	_vertexArray.resize(parameter.numParticle);

	// �������x�x�N�g����������x�����_���ŕ��U�����č쐬����
	_initVelocityArray.resize(parameter.numParticle);

	//TODO: lerp�̓V�F�[�_�̕��������炵�����Ƃ肠����CPU����
	srand((unsigned int)time(nullptr));

	for (int i = 0; i < _parameter.numParticle; ++i)
	{
		float t1 = (float)rand() / RAND_MAX;
		float t2 = (float)rand() / RAND_MAX;
		float t3 = (float)rand() / RAND_MAX;
		// TODO:������ւ�̕��U�͓K���B���������̂��p�����[�^�Őݒ�ł���悤�ɂ�����
		float theta = PI_OVER2 / 8.0f * t1;
		float phi = PI_OVER2 * 4.0f * t2;
		float absVelocity = _parameter.initVelocity * (0.9f * t3 + 1.1f * (1.0f - t3));

		Vec3 initVelocity;
		initVelocity.x = sinf(theta) * cosf(phi) * absVelocity;
		initVelocity.y = cosf(theta) * absVelocity;
		initVelocity.x = sinf(theta) * sinf(phi) * absVelocity;

		_initVelocityArray[i] = initVelocity;
	}

	// �Ƃ肠�����ݒ�ʂ̃p�[�e�B�N�����ӂ��������炻��ŏI���ɂ���
	// TODO:�ӂ��������p�[�e�B�N�����ė��p����悤�ɂ�����
	_glData = createOpenGLProgram(
		// vertex shader
		"attribute vec4 a_position;"
		"attribute vec3 a_initVelocity;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_viewMatrix;"
		"uniform mat4 u_projectionMatrix;"
		"uniform vec3 u_gravity;"
		"uniform float u_elapsedTime;"
		"uniform float u_lifeTime;"
		"uniform float u_pointSize;"
		"varying float v_opacity;"
		"void main()"
		"{"
		"	vec4 position = a_position;"
		"	position.xyz += a_initVelocity * u_elapsedTime + u_gravity * u_elapsedTime * u_elapsedTime / 2.0;"
		"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * position;"
		"	v_opacity = 1.0 - u_elapsedTime / u_lifeTime;"
		"	gl_PointSize = u_pointSize;"
		"}"
		,
		// fragment shader
		"uniform sampler2D u_texture;"
		"varying float v_opacity;"
		"void main()"
		"{"
		"	gl_FragColor = texture2D(u_texture, gl_PointCoord);"
		"	gl_FragColor.a *= v_opacity;"
		"	if (gl_FragColor.a == 0.0)"
		"	{"
		"		discard;"
		"	}"
		"}"
	);

	_glData.uniformTexture = glGetUniformLocation(_glData.shaderProgram, "u_texture");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_glData.uniformTexture < 0)
	{
		return false;
	}

	_uniformGravity = glGetUniformLocation(_glData.shaderProgram, "u_gravity");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_uniformGravity < 0)
	{
		return false;
	}

	_uniformLifeTime = glGetUniformLocation(_glData.shaderProgram, "u_lifeTime");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_uniformLifeTime < 0)
	{
		return false;
	}

	_attributeInitVelocity = glGetAttribLocation(_glData.shaderProgram, "a_initVelocity");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_attributeInitVelocity < 0)
	{
		return false;
	}

	_uniformPointSize = glGetUniformLocation(_glData.shaderProgram, "u_pointSize");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_uniformPointSize < 0)
	{
		return false;
	}

	_uniformElapsedTime = glGetUniformLocation(_glData.shaderProgram, "u_elapsedTime");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_uniformElapsedTime < 0)
	{
		return false;
	}

	return true;
}

void Particle3D::update(float dt)
{
	_elapsedTime += dt;
}

void Particle3D::renderWithShadowMap()
{
	glEnable(GL_DEPTH_TEST);

	glUseProgram(_glData.shaderProgram);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glUniform3f(_glData.uniformMultipleColor, getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glUniformMatrix4fv(_glData.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glUniform3fv(_uniformGravity, 1, (GLfloat*)&_parameter.gravity);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glUniform1f(_uniformLifeTime, _parameter.lifeTime);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	glUniform1f(_uniformPointSize, _parameter.pointSize);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	glUniform1f(_uniformElapsedTime, _elapsedTime);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	glEnableVertexAttribArray(_attributeInitVelocity);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
	glVertexAttribPointer(_attributeInitVelocity, sizeof(_initVelocityArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_initVelocityArray[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

	glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
	glDrawArrays(GL_POINTS, 0, _parameter.numParticle);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
}

} // namespace mgrrenderer
