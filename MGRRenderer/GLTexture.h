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
	//bool initWithTexture(GLuint textureId, const Size& contentSize, TextureUtility::PixelFormat format);
	bool initDepthTexture(const Size& contentSize);

	GLuint getTextureId() const { return _textureId; }
	// デプステクスチャやレンダーテクスチャとして使用するときに使う
	GLuint getFrameBufferId() const { return _frameBufferId; }

private:
	GLuint _textureId;
	GLuint _frameBufferId;
	bool _hasPremultipliedAlpha;
};

} // namespace mgrrenderer
#endif
