#include "Sprite2D.h"
#include "FileUtility.h"
#include "Image.h"
#include "Director.h"
#include "Shaders.h"

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

	Texture* texture = new Texture(); // Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
	texture->initWithImage(image);

	return initWithTexture(texture->getTextureId(), texture->getContentSize(), Texture::getDefaultPixelFormat());
}

bool Sprite2D::initWithTexture(GLuint textureId, const Size& contentSize, Texture::PixelFormat format)
{
	_texture = new Texture(); // Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
	_texture->initWithTexture(textureId, contentSize, format);

	//TODO: ��Z���钸�_�J���[�ɂ͑Ή����Ȃ�

	_glProgram.initWithShaderString(shader::VERTEX_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR, shader::FRAGMENT_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR);

	_glProgram.attributeTextureCoordinates = glGetAttribLocation(_glProgram.shaderProgram, "a_texCoord");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_glProgram.attributeTextureCoordinates < 0)
	{
		return false;
	}

	_glProgram.uniformTexture = glGetUniformLocation(_glProgram.shaderProgram, "u_texture");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_glProgram.uniformTexture < 0)
	{
		return false;
	}

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

void Sprite2D::renderWithShadowMap()
{
	_renderCommand.init([=]
	{
		glDisable(GL_DEPTH_TEST);

		// cocos2d-x��TriangleCommand���s���Ă�`������ȁB�B�e�N�X�`���o�C���h��Texture2D�ł���Ă�̂ɑ��v���H
		glUseProgram(_glProgram.shaderProgram);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniform3f(_glProgram.uniformMultipleColor, getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgram.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glEnableVertexAttribArray(_glProgram.attributeTextureCoordinates);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL�����ŃG���[���� glGetError()=%d", glGetError());

		glVertexAttribPointer((GLuint)AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.position);
		glVertexAttribPointer(_glProgram.attributeTextureCoordinates, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.textureCoordinate);

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
