#include "Particle3D.h"
#include "Director.h"
#include "Texture.h"
#include "Image.h"

namespace mgrrenderer
{

Particle3D::Particle3D() :
_texture(nullptr),
_elapsedTime(0.0f),
_uniformGravity(-1),
_uniformLifeTime(-1),
_uniformInitVelocity(-1),
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
	// 一個でもParticle3Dがあったらポイントスプライトを有効にする。これを有効にしないとgl_PointCoordが有効に働かない
	glEnable(GL_POINT_SPRITE);

	_parameter = parameter;

	Logger::logAssert(!parameter.textureFilePath.empty(), "テクスチャパスが空。");

	Image image; // ImageはCPU側のメモリを使っているのでこのスコープで解放されてもよいものだからスタックに取る
	image.initWithFilePath(parameter.textureFilePath);

	_texture = new Texture(); // TextureはGPU側のメモリを使ってるので解放されると困るのでヒープにとる
	_texture->initWithImage(image);

	// vec3で埋まるので、初期値の(0,0,0)で埋まっている
	_vertexArray.resize(parameter.numParticle);

	// とりあえず設定量のパーティクルをふきだしたらそれで終わりにする
	// TODO:ふきだしたパーティクルを再利用するようにしたい
	_glData = createOpenGLProgram(
		// vertex shader
		"attribute vec4 a_position;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_viewMatrix;"
		"uniform mat4 u_projectionMatrix;"
		"uniform vec3 u_gravity;"
		"uniform float u_elapsedTime;"
		"uniform float u_lifeTime;"
		"uniform vec3 u_initVelocity;"
		"uniform float u_pointSize;"
		"varying float v_opacity;"
		"void main()"
		"{"
		"	vec4 position = a_position;"
		"	position.xyz += u_initVelocity * u_elapsedTime + u_gravity * u_elapsedTime * u_elapsedTime / 2.0;"
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

	_uniformInitVelocity = glGetUniformLocation(_glData.shaderProgram, "u_initVelocity");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_uniformInitVelocity < 0)
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
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glUniform3f(_glData.uniformMultipleColor, getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glUniformMatrix4fv(_glData.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glUniform3fv(_uniformGravity, 1, (GLfloat*)&_parameter.gravity);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glUniform3fv(_uniformInitVelocity, 1, (GLfloat*)&_parameter.initVelocity);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glUniform1f(_uniformLifeTime, _parameter.lifeTime);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glUniform1f(_uniformPointSize, _parameter.pointSize);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	glUniform1f(_uniformElapsedTime, _elapsedTime);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

	glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
	glDrawArrays(GL_POINTS, 0, _vertexArray.size());
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
}

} // namespace mgrrenderer
