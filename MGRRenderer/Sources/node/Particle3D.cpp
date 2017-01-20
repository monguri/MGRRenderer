#include "Particle3D.h"
#include "renderer/Director.h"
#include "renderer/Image.h"
// srand関数のため
#include <time.h>
#if defined(MGRRENDERER_USE_OPENGL)
#include "renderer/GLTexture.h"
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
#if defined(MGRRENDERER_USE_DIRECT3D)
	(void)parameter; // 未使用変数警告抑制
#elif defined(MGRRENDERER_USE_OPENGL)
	// 一個でもParticle3Dがあったらポイントスプライトを有効にする。これを有効にしないとgl_PointCoordが有効に働かない
	glEnable(GL_POINT_SPRITE);

	_parameter = parameter;

	Logger::logAssert(!parameter.textureFilePath.empty(), "テクスチャパスが空。");

	Image image; // ImageはCPU側のメモリを使っているのでこのスコープで解放されてもよいものだからスタックに取る
	image.initWithFilePath(parameter.textureFilePath);

	_texture = new GLTexture(); // TextureはGPU側のメモリを使ってるので解放されると困るのでヒープにとる
	static_cast<Texture*>(_texture)->initWithImage(image); // TODO:なぜか暗黙に継承元クラスのメソッドが呼べない

	// vec3で埋まるので、初期値の(0,0,0)で埋まっている
	int numParticle = static_cast<int>(_parameter.loopFlag ? parameter.numParticle * parameter.lifeTime : parameter.numParticle);
	_vertexArray.resize(numParticle);
	_initVelocityArray.resize(numParticle);
	_elapsedTimeArray.resize(numParticle);

	//TODO: lerpはシェーダの方が光束らしいがとりあえずCPU側で
	// 初期速度ベクトルをある程度ランダムで分散させて作成する
	srand((unsigned int)time(nullptr));

	for (int i = 0; i < numParticle; ++i)
	{
		float t1 = (float)rand() / RAND_MAX;
		float t2 = (float)rand() / RAND_MAX;
		float t3 = (float)rand() / RAND_MAX;
		// TODO:ここらへんの分散は適当。こういうのもパラメータで設定できるようにしたい
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
			// lifeTime以上であることが、パーティクルが再利用可能になっている条件。lifeTimeのときはオパシティは0になっている
			_elapsedTimeArray[i] = _parameter.lifeTime;
		}
		else
		{
			_elapsedTimeArray[i] = 0.0f;
		}
	}

	// とりあえず設定量のパーティクルをふきだしたらそれで終わりにする
	_glProgram.initWithShaderString(
		// vertex shader
		"#version 430\n"
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
		"#version 430\n"
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
		// %演算子をつかうために1000倍してmsで扱う
		_elapsedTimeMs += static_cast<int>(dt * 1000);

		int timePerEmit = (int)(1.0f / _parameter.numParticle * 1000);

		if (_elapsedTimeMs > timePerEmit)
		{
			int numParticle = static_cast<int>(_parameter.numParticle * _parameter.lifeTime);
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
				// lifeTimeを超えたものは再利用する
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

#if defined(MGRRENDERER_DEFFERED_RENDERING)
void Particle3D::renderGBuffer()
{
	Node::renderGBuffer();
}
#endif

void Particle3D::renderForward()
{
	_renderCommand.init([=]
	{
#if defined(MGRRENDERER_USE_OPENGL)
		glUseProgram(_glProgram.getShaderProgram());
		GLProgram::checkGLError();

		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		GLProgram::checkGLError();

		glUniform3fv(_glProgram.getUniformLocation("u_gravity"), 1, (GLfloat*)&_parameter.gravity);
		GLProgram::checkGLError();

		glUniform1f(_glProgram.getUniformLocation("u_lifeTime"), _parameter.lifeTime);
		GLProgram::checkGLError();
		glUniform1f(_glProgram.getUniformLocation("u_pointSize"), _parameter.pointSize);
		GLProgram::checkGLError();

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		GLProgram::checkGLError();
		glEnableVertexAttribArray(_glProgram.getAttributeLocation("a_initVelocity"));
		GLProgram::checkGLError();
		glEnableVertexAttribArray(_glProgram.getAttributeLocation("a_elapsedTime"));
		GLProgram::checkGLError();

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_vertexArray.data());
		GLProgram::checkGLError();
		glVertexAttribPointer(_glProgram.getAttributeLocation("a_initVelocity"), sizeof(_initVelocityArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_initVelocityArray.data());
		GLProgram::checkGLError();
		glVertexAttribPointer(_glProgram.getAttributeLocation("a_elapsedTime"), sizeof(_elapsedTimeArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)_elapsedTimeArray.data());
		GLProgram::checkGLError();

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());

		int numParticle = static_cast<int>(_parameter.loopFlag ? _parameter.numParticle * _parameter.lifeTime : _parameter.numParticle);
		glDrawArrays(GL_POINTS, 0, numParticle);
		GLProgram::checkGLError();
#endif
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
