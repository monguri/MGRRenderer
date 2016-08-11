#pragma once
#include "Node.h"
#include "GLProgram.h"
#include "CustomRenderCommand.h"
#include "Director.h"

namespace mgrrenderer
{

class GLTexture;

class LabelAtlas : public Node
{
public:
	friend Director; // G�o�b�t�@�̃f�o�b�O�`��̂���Director�ɂ͌��J���Ă���

	LabelAtlas();
	~LabelAtlas();
#if defined(MGRRENDERER_USE_OPENGL)
	bool init(const std::string& string, const GLTexture* texture, int itemWidth, int itemHeight, char mapStartChararcter);
	void setString(const std::string& string);
#endif

private:
#if defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
	const GLTexture* _texture;
#endif
	CustomRenderCommand _renderCommand;
	char _mapStartCharacter;
	std::string _string;
	std::vector<Position2DTextureCoordinates> _vertices;
	std::vector<unsigned short> _indices;
	int _itemWidth;
	int _itemHeight;

	void renderGBuffer() override;
	void renderForward() override;
};

} // namespace mgrrenderer
