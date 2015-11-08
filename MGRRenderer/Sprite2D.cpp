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

bool Sprite2D::init(const Vec2& position, const std::string& filePath)
{
	// Textureをロードし、pngやjpegを生データにし、OpenGLにあげる仕組みを作らねば。。Spriteのソースを見直すときだ。
	Image image; // ImageはCPU側のメモリを使っているのでこのスコープで解放されてもよいものだからスタックに取る
	image.initWithFilePath(filePath);

	_texture = new Texture(); // TextureはGPU側のメモリを使ってるので解放されると困るのでヒープにとる
	_texture->initWithImage(image);

	_rect = Rect(position.x, position.y, _texture->getContentSize().width, _texture->getContentSize().height);

	//TODO: 乗算する頂点カラーには対応しない

	_glData = createOpenGLProgram(
		// vertex shader
		"attribute mediump vec4 attr_pos;"
		"attribute mediump vec2 attr_texCoord;"
		"varying mediump vec2 vary_texCoord;"
		"uniform mediump mat4 unif_view_mat;"
		"uniform mediump mat4 unif_proj_mat;"
		"void main()"
		"{"
		"	gl_Position = unif_proj_mat * unif_view_mat * attr_pos;"
		"	vary_texCoord = attr_texCoord;"
		"}"
		,
		// fragment shader
		"uniform sampler2D texture;"
		"varying mediump vec2 vary_texCoord;"
		"void main()"
		"{"
		"	gl_FragColor = texture2D(texture, vary_texCoord);" // テクスチャ番号は0のみに対応
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

	//glUniform1i(_glData.uniformTexture, 0); // TODO:ここに書くのが適切か？Texture.cppにglActiveとBindを書いてるのもそうだが
	_quadrangle.bottomLeft.pos = Vec2(position.x, position.y);
	_quadrangle.bottomLeft.texCoords = TextureCoordinates(0, 1);
	_quadrangle.bottomRight.pos = Vec2(position.x + _rect.size.width, position.y);
	_quadrangle.bottomRight.texCoords = TextureCoordinates(1, 1);
	_quadrangle.topLeft.pos = Vec2(position.x, position.y + _rect.size.height);
	_quadrangle.topLeft.texCoords = TextureCoordinates(0, 0);
	_quadrangle.topRight.pos = Vec2(position.x + _rect.size.width, position.y + _rect.size.height);
	_quadrangle.topRight.texCoords = TextureCoordinates(1, 0);

	return true;
}

void Sprite2D::render()
{
	// cocos2d-xはTriangleCommand発行してる形だからな。。テクスチャバインドはTexture2Dでやってるのに大丈夫か？
	glUseProgram(_glData.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray(_glData.attributeVertexPosition);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray(_glData.attributeTextureCoordinates);
	assert(glGetError() == GL_NO_ERROR);

	glVertexAttribPointer(_glData.attributeVertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.pos);
	glVertexAttribPointer(_glData.attributeTextureCoordinates, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.texCoords);

	glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

} // namespace mgrrenderer
