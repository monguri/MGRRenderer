#pragma once

#include "Config.h"
#if defined(MGRRENDERER_USE_OPENGL)

#include "Texture.h"

namespace mgrrenderer
{

class GLTexture : public Texture
{
public:
	GLTexture();
	~GLTexture();
	bool initWithImage(const Image& image, TextureUtility::PixelFormat format) override;
	bool initDepthTexture(GLenum textureUnit, const SizeUint& contentSize);
	bool initDepthCubeMapTexture(GLenum textureUnit, unsigned int size);
	bool initRenderTexture(GLenum pixelFormat, const SizeUint& contentSize);

	GLuint getTextureId() const { return _textureId; }

private:
	GLuint _textureId;
	bool _hasPremultipliedAlpha;
};

} // namespace mgrrenderer
#endif
