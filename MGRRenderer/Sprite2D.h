#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "CustomRenderCommand.h"
#include <string>
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DProgram.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "GLProgram.h"
#endif

namespace mgrrenderer
{

#if defined(MGRRENDERER_USE_DIRECT3D)
class D3DTexture;
#elif defined(MGRRENDERER_USE_OPENGL)
class GLTexture;
#endif

namespace TextureUtility
{
enum class PixelFormat : int;
} // TextureUtility

class Sprite2D :
	public Node
{
public:
	Sprite2D();
	virtual ~Sprite2D();
	bool init(const std::string& filePath);
#if defined(MGRRENDERER_USE_OPENGL)
	bool initWithTexture(GLuint textureId, const Size& contentSize, TextureUtility::PixelFormat format);
	const GLTexture* getTexture() const { return _texture; }
#endif

protected:
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgram;
	ID3D11Buffer* _vertexBuffer;
	ID3D11Buffer* _indexBuffer;
	ID3D11InputLayout* _inputLayout;
	ID3D11Buffer* _constantBuffers[4];
	D3DTexture* _texture;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
	GLTexture* _texture;
#endif
	CustomRenderCommand _renderCommand;
	Quadrangle2D _quadrangle;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
