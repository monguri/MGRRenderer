#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "Texture.h"
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

protected:
	void renderWithShadowMap() override;

private:
	OpenGLProgramData _glData;
	Texture* _texture;
	Quadrangle2D _quadrangle;
};

} // namespace mgrrenderer
