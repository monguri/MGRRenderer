#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "Texture.h"
#include "CustomRenderCommand.h"
#include <string>

namespace mgrrenderer
{

class Sprite2D :
	public Node
{
public:
	Sprite2D();
	~Sprite2D();
	bool init(const std::string& filePath);
	bool initWithTexture(GLuint textureId, const Size& contentSize, Texture::PixelFormat format);
	const Texture* getTexture() const { return _texture; }

protected:
	OpenGLProgramData _glData;
	CustomRenderCommand _renderCommand;
	Texture* _texture;
	Quadrangle2D _quadrangle;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
