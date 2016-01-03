#include "LabelAtlas.h"
#include "Director.h"
#include "Texture.h"

namespace mgrrenderer
{

LabelAtlas::LabelAtlas() :
_mapStartCharacter(0),
_itemWidth(0),
_itemHeight(0),
_texture(nullptr)
{
}

LabelAtlas::~LabelAtlas()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	_texture = nullptr;
}

bool LabelAtlas::init(const std::string& string, const Texture* texture, int itemWidth, int itemHeight, char mapStartChararcter)
{
	glBindTexture(GL_TEXTURE_2D, 0);
	_texture = texture;

	_mapStartCharacter = mapStartChararcter;
	_itemWidth = itemWidth;
	_itemHeight = itemHeight;

	setString(string);

	_glData = createOpenGLProgram(
		// vertex shader
		"attribute vec4 a_position;"
		"attribute vec2 a_texCoord;"
		"varying vec2 v_texCoord;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_viewMatrix;"
		"uniform mat4 u_projectionMatrix;"
		"void main()"
		"{"
		"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * a_position;"
		"	v_texCoord = a_texCoord;"
		"}"
		,
		// fragment shader
		"uniform sampler2D texture;"
		"varying vec2 v_texCoord;"
		"void main()"
		"{"
		"	gl_FragColor = texture2D(texture, v_texCoord);" // テクスチャ番号は0のみに対応
		"}"
		);

	_glData.attributeTextureCoordinates = glGetAttribLocation(_glData.shaderProgram, "a_texCoord");
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

	return true;
}

void LabelAtlas::setString(const std::string& string)
{
	_string = string;

	// setStringの回数は少ないという前提のもと、以下は毎回計算している
	int itemsPerRow = _texture->getContentSize().width / _itemWidth;

	float itemWidthOnTexCoord = _itemWidth / _texture->getContentSize().width;
	float itemHeightOnTexCoord = _itemHeight / _texture->getContentSize().height;

	size_t len = _string.length();
	_quadrangles.clear();
	_quadrangles.resize(len);

	for (size_t i = 0; i < len; ++i)
	{
		unsigned char a = (unsigned char)_string[i] - _mapStartCharacter;
		char row = a % itemsPerRow;
		char column = a / itemsPerRow;

		float left = row * itemWidthOnTexCoord;
		float right = left + itemWidthOnTexCoord;
		float top = column * itemHeightOnTexCoord;
		float bottom = top + itemHeightOnTexCoord;

		_quadrangles[i].topLeft.textureCoordinate = Vec2(left, top);
		_quadrangles[i].topRight.textureCoordinate = Vec2(right, top);
		_quadrangles[i].bottomLeft.textureCoordinate = Vec2(left, bottom);
		_quadrangles[i].bottomRight.textureCoordinate = Vec2(right, bottom);

		// カーニングは考慮しない。フォントファイルまだ用意してないし
		_quadrangles[i].topLeft.position = Vec2(i * _itemWidth, _itemHeight);
		_quadrangles[i].topRight.position = Vec2((i + 1) * _itemWidth, _itemHeight);
		_quadrangles[i].bottomLeft.position = Vec2(i * _itemWidth, 0.0f);
		_quadrangles[i].bottomRight.position = Vec2((i + 1) * _itemWidth, 0.0f);
	}
}

void LabelAtlas::render()
{
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

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangles[0].topLeft.position);
	glVertexAttribPointer(_glData.attributeTextureCoordinates, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangles[0].topLeft.textureCoordinate);

	glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
	glDrawArrays(GL_QUADS, 0, _quadrangles.size()); // TODO:簡単なのでGL_QUADSで指定。最終的にはVBOに。
}

} // namespace mgrrenderer
