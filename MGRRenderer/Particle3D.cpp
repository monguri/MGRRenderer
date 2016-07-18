#include "Particle3D.h"
#include "Director.h"
#include "Image.h"
// srand�֐��̂���
#include <time.h>
#if defined(MGRRENDERER_USE_OPENGL)
#include "GLTexture.h"
#endif

namespace mgrrenderer
{

Particle3D::Particle3D() :
#if defined(MGRRENDERER_USE_OPENGL)
_texture(nullptr),
#endif
_elapsedTimeMs(0)
{
}

Particle3D::~Particle3D()
{
#if defined(MGRRENDERER_USE_OPENGL)
	glBindTexture(GL_TEXTURE_2D, 0);

	if (_texture)
	{
		delete _texture;
		_texture = nullptr;
	}
#endif
}

bool Particle3D::initWithParameter(const Particle3D::Parameter& parameter)
{
#if defined(MGRRENDERER_USE_OPENGL)
	// ��ł�Particle3D����������|�C���g�X�v���C�g��L���ɂ���B�����L���ɂ��Ȃ���gl_PointCoord���L���ɓ����Ȃ�
	glEnable(GL_POINT_SPRITE);

	_parameter = parameter;

	Logger::logAssert(!parameter.textureFilePath.empty(), "�e�N�X�`���p�X����B");

	Image image; // Image��CPU���̃��������g���Ă���̂ł��̃X�R�[�v�ŉ������Ă��悢���̂�����X�^�b�N�Ɏ��
	image.initWithFilePath(parameter.textureFilePath);

	_texture = new GLTexture(); // Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
	static_cast<Texture*>(_texture)->initWithImage(image); // TODO:�Ȃ����ÖقɌp�����N���X�̃��\�b�h���ĂׂȂ�

	// vec3�Ŗ��܂�̂ŁA�����l��(0,0,0)�Ŗ��܂��Ă���
	int numParticle = _parameter.loopFlag ? parameter.numParticle * parameter.lifeTime : parameter.numParticle;
	_vertexArray.resize(numParticle);
	_initVelocityArray.resize(numParticle);
	_elapsedTimeArray.resize(numParticle);

	//TODO: lerp�̓V�F�[�_�̕��������炵�����Ƃ肠����CPU����
	// �������x�x�N�g����������x�����_���ŕ��U�����č쐬����
	srand((unsigned int)time(nullptr));

	for (int i = 0; i < numParticle; ++i)
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
		initVelocity.z = sinf(theta) * sinf(phi) * absVelocity;

		_initVelocityArray[i] = initVelocity;

		if (_parameter.loopFlag)
		{
			// lifeTime�ȏ�ł��邱�Ƃ��A�p�[�e�B�N�����ė��p�\�ɂȂ��Ă�������BlifeTime�̂Ƃ��̓I�p�V�e�B��0�ɂȂ��Ă���
			_elapsedTimeArray[i] = _parameter.lifeTime;
		}
		else
		{
			_elapsedTimeArray[i] = 0.0f;
		}
	}

	// �Ƃ肠�����ݒ�ʂ̃p�[�e�B�N�����ӂ��������炻��ŏI���ɂ���
	_glProgram.initWithShaderString(
		// vertex shader
		"attribute vec4 a_position;"
		"attribute vec3 a_initVelocity;"
		"attribute float a_elapsedTime;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_viewMatrix;"
		"uniform mat4 u_projectionMatrix;"
		"uniform vec3 u_gravity;"
		"uniform float u_lifeTime;"
		"uniform float u_pointSize;"
		"varying float v_opacity;"
		"void main()"
		"{"
		"	vec4 position = a_position;"
		"	position.xyz += a_initVelocity * a_elapsedTime + u_gravity * a_elapsedTime * a_elapsedTime / 2.0;"
		"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * position;"
		"	v_opacity = 1.0 - a_elapsedTime / u_lifeTime;"
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
#endif

	return true;
}

void Particle3D::update(float dt)
{
	if (_parameter.loopFlag)
	{
		// %���Z�q���������߂�1000�{����ms�ň���
		_elapsedTimeMs += dt * 1000;

		int timePerEmit = (int)(1.0f / _parameter.numParticle * 1000);

		if (_elapsedTimeMs > timePerEmit)
		{
			int numParticle = _parameter.numParticle * _parameter.lifeTime;
			for (int i = 0; i < numParticle; ++i)
			{
				_elapsedTimeArray[i] += dt;
			}

			int numEmit = _elapsedTimeMs / timePerEmit;
			if (numEmit <= 0)
			{
				return;
			}

			_elapsedTimeMs %= timePerEmit;

			int emitCount = 0;
			for (int i = 0; i < numParticle; ++i)
			{
				// lifeTime�𒴂������͍̂ė��p����
				if (_elapsedTimeArray[i] > _parameter.lifeTime)
				{
					_elapsedTimeArray[i] = 0;
					emitCount++;
					if (emitCount >= numEmit)
					{
						break;
					}
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < _parameter.numParticle; ++i)
		{
			_elapsedTimeArray[i] += dt;
		}
	}
}

void Particle3D::renderGBuffer()
{
	Node::renderGBuffer();
}

void Particle3D::renderWithShadowMap()
{
	_renderCommand.init([=]
	{
#if defined(MGRRENDERER_USE_OPENGL)
		glEnable(GL_DEPTH_TEST);

		glUseProgram(_glProgram.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniform3fv(_glProgram.getUniformLocation("u_gravity"), 1, (GLfloat*)&_parameter.gravity);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniform1f(_glProgram.getUniformLocation("u_lifeTime"), _parameter.lifeTime);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		glUniform1f(_glProgram.getUniformLocation("u_pointSize"), _parameter.pointSize);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		glEnableVertexAttribArray(_glProgram.getAttributeLocation("a_initVelocity"));
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		glEnableVertexAttribArray(_glProgram.getAttributeLocation("a_elapsedTime"));
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		glVertexAttribPointer(_glProgram.getAttributeLocation("a_initVelocity"), sizeof(_initVelocityArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_initVelocityArray.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
		glVertexAttribPointer(_glProgram.getAttributeLocation("a_elapsedTime"), sizeof(_elapsedTimeArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_elapsedTimeArray.data());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());

		int numParticle = _parameter.loopFlag ? _parameter.numParticle * _parameter.lifeTime : _parameter.numParticle;
		glDrawArrays(GL_POINTS, 0, numParticle);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());
#endif
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
