#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "GLProgram.h"
#include "Texture.h"
#include "CustomRenderCommand.h"
#include <string>

#if defined(MGRRENDERER_USE_OPENGL)
namespace mgrrenderer
{

class Sprite2D :
	public Node
{
public:
	Sprite2D();
	virtual ~Sprite2D();
	bool init(const std::string& filePath);
	bool initWithTexture(GLuint textureId, const Size& contentSize, Texture::PixelFormat format);
	const Texture* getTexture() const { return _texture; }

protected:
	GLProgram _glProgram;
	CustomRenderCommand _renderCommand;
	Texture* _texture;
	Quadrangle2D _quadrangle;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
#endif
