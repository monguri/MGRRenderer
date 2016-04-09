#pragma once
#include "Node.h"
#include "GLProgram.h"
#include "CustomRenderCommand.h"

namespace mgrrenderer
{

class Texture;

class LabelAtlas : public Node
{
public:
	LabelAtlas();
	~LabelAtlas();
	bool init(const std::string& string, const Texture* texture, int itemWidth, int itemHeight, char mapStartChararcter);
	void setString(const std::string& string);
	void renderWithShadowMap() override;

private:
#if defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
#endif
	CustomRenderCommand _renderCommand;
	char _mapStartCharacter;
	std::string _string;
	const Texture* _texture;
	std::vector<Position2DTextureCoordinates> _vertices;
	std::vector<unsigned short> _indices;
	int _itemWidth;
	int _itemHeight;
};

} // namespace mgrrenderer
