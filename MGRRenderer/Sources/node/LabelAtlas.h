#pragma once
#include "Node.h"
#include "renderer/GLProgram.h"
#include "renderer/CustomRenderCommand.h"
#include "renderer/Director.h"

namespace mgrrenderer
{

class GLTexture;

class LabelAtlas : public Node
{
public:
	friend Director; // Gバッファのデバッグ描画のためDirectorには公開している

	LabelAtlas();
	~LabelAtlas();
#if defined(MGRRENDERER_USE_OPENGL)
	bool init(const std::string& string, const GLTexture* texture, float itemWidth, float itemHeight, char mapStartChararcter);
	void setString(const std::string& string);
#endif

private:
#if defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgramForForwardRendering;
	const GLTexture* _texture;
#endif
	CustomRenderCommand _renderForwardCommand;
	char _mapStartCharacter;
	std::string _string;
	std::vector<Position2DTextureCoordinates> _vertices;
	std::vector<unsigned short> _indices;
	float _itemWidth;
	float _itemHeight;

#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void renderGBuffer() override;
#endif
	void renderForward() override;
};

} // namespace mgrrenderer
