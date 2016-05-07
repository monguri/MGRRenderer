#include "LabelAtlas.h"
#include "Director.h"
#include "GLTexture.h"
#include "Shaders.h"

namespace mgrrenderer
{

LabelAtlas::LabelAtlas() :
#if defined(MGRRENDERER_USE_OPENGL)
_texture(nullptr),
#endif
_mapStartCharacter(0),
_itemWidth(0),
_itemHeight(0)
{
}

LabelAtlas::~LabelAtlas()
{
#if defined(MGRRENDERER_USE_OPENGL)
	glBindTexture(GL_TEXTURE_2D, 0);
	_texture = nullptr;
#endif
}

#if defined(MGRRENDERER_USE_OPENGL)
bool LabelAtlas::init(const std::string& string, const GLTexture* texture, int itemWidth, int itemHeight, char mapStartChararcter)
{
	_texture = texture;

	_mapStartCharacter = mapStartChararcter;
	_itemWidth = itemWidth;
	_itemHeight = itemHeight;

	setString(string);

	_glProgram.initWithShaderString(shader::VERTEX_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR, shader::FRAGMENT_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR);

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
	_vertices.clear();
	_vertices.resize(4 * len); // GL_TRIANGLESを使うが4角形内の重複はインデックスで排除するので4頂点
	_indices.clear();
	_indices.resize(6 * len); // GL_TRIANGLESを使うので一つの4角形に6

	for (size_t i = 0; i < len; ++i)
	{
		unsigned char a = (unsigned char)_string[i] - _mapStartCharacter;
		char column = a % itemsPerRow;
		char row = a / itemsPerRow;

		float left = column * itemWidthOnTexCoord;
		float right = left + itemWidthOnTexCoord;
		float top = row * itemHeightOnTexCoord;
		float bottom = top + itemHeightOnTexCoord;

		_vertices[4 * i].textureCoordinate = Vec2(left, top);
		_vertices[4 * i + 1].textureCoordinate = Vec2(left, bottom);
		_vertices[4 * i + 2].textureCoordinate = Vec2(right, top);
		_vertices[4 * i + 3].textureCoordinate = Vec2(right, bottom);

		// カーニングは考慮しない。フォントファイルまだ用意してないし
		_vertices[4 * i].position = Vec2(i * _itemWidth, _itemHeight);
		_vertices[4 * i + 1].position = Vec2(i * _itemWidth, 0.0f);
		_vertices[4 * i + 2].position = Vec2((i + 1) * _itemWidth, _itemHeight);
		_vertices[4 * i + 3].position = Vec2((i + 1) * _itemWidth, 0.0f);

		// TODO:glDrawElementsを使っていて、インデックスは単純に増やしているだけで頂点の重複は考慮してない。もっと効率いいやり方あるかも
		_indices[6 * i] = 4 * i;
		_indices[6 * i + 1] = 4 * i + 1;
		_indices[6 * i + 2] = 4 * i + 2;
		_indices[6 * i + 3] = 4 * i + 3;
		_indices[6 * i + 4] = 4 * i + 2;
		_indices[6 * i + 5] = 4 * i + 1;
	}
}
#endif

void LabelAtlas::renderWithShadowMap()
{
	_renderCommand.init([=]
	{
#if defined(MGRRENDERER_USE_OPENGL)
		if (_indices.size() == 0)
		{
			// まだ文字設定をしてないときは描画しない。描画するとglVertexAttribPointerで0インデックスにアクセスするのでエラーになる。
			return;
		}

		glDisable(GL_DEPTH_TEST);

		glUseProgram(_glProgram.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniform3f(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());


		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_vertices[0].position);
		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, &_indices[0]);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
#endif
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
