#include "Sprite2D.h"
#include "FileUtility.h"
#include "Image.h"
#include "Director.h"
#include <assert.h>

namespace mgrrenderer
{

Sprite2D::Sprite2D() : _texture(nullptr)
{
}

Sprite2D::~Sprite2D()
{
	glBindTexture(GL_TEXTURE_2D, 0);
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

	_texture = new Texture(); // Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
	_texture->initWithImage(image);

	//TODO: ��Z���钸�_�J���[�ɂ͑Ή����Ȃ�

	_glData = createOpenGLProgram(
		// vertex shader
		"attribute vec4 attr_position;"
		"attribute vec2 attr_texCoord;"
		"varying vec2 vary_texCoord;"
		"uniform mat4 unif_modelMatrix;"
		"uniform mat4 unif_viewMatrix;"
		"uniform mat4 unif_projectionMatrix;"
		"void main()"
		"{"
		"	gl_Position = unif_projectionMatrix * unif_viewMatrix * unif_modelMatrix * attr_position;"
		"	vary_texCoord = attr_texCoord;"
		"}"
		,
		// fragment shader
		"uniform sampler2D texture;"
		"varying vec2 vary_texCoord;"
		"void main()"
		"{"
		"	gl_FragColor = texture2D(texture, vary_texCoord);" // �e�N�X�`���ԍ���0�݂̂ɑΉ�
		"}"
		);

	_glData.attributeTextureCoordinates = glGetAttribLocation(_glData.shaderProgram, "attr_texCoord");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_glData.attributeTextureCoordinates < 0)
	{
		return false;
	}

	_glData.uniformTexture = glGetUniformLocation(_glData.shaderProgram, "texture");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_glData.uniformTexture < 0)
	{
		return false;
	}

	_quadrangle.bottomLeft.position = Vec2(0.0f, 0.0f);
	_quadrangle.bottomLeft.textureCoordinate = Vec2(0.0f, 1.0f);
	_quadrangle.bottomRight.position = Vec2(_texture->getContentSize().width, 0.0f);
	_quadrangle.bottomRight.textureCoordinate = Vec2(1.0f, 1.0f);
	_quadrangle.topLeft.position = Vec2(0.0f, _texture->getContentSize().height);
	_quadrangle.topLeft.textureCoordinate = Vec2(0.0f, 0.0f);
	_quadrangle.topRight.position = Vec2(_texture->getContentSize().width, _texture->getContentSize().height);
	_quadrangle.topRight.textureCoordinate = Vec2(1.0f, 0.0f);

	return true;
}

void Sprite2D::render()
{
	// cocos2d-x��TriangleCommand���s���Ă�`������ȁB�B�e�N�X�`���o�C���h��Texture2D�ł���Ă�̂ɑ��v���H
	glUseProgram(_glData.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	glUniformMatrix4fv(_glData.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray(_glData.attributeTextureCoordinates);
	assert(glGetError() == GL_NO_ERROR);

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.position);
	glVertexAttribPointer(_glData.attributeTextureCoordinates, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.textureCoordinate);

	glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

} // namespace mgrrenderer
