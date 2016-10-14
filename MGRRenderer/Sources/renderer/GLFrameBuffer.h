#pragma once

#include "Config.h"
#include <vector>
#if defined(MGRRENDERER_USE_OPENGL)
#include <gles/include/glew.h>
#include "BasicDataTypes.h"

namespace mgrrenderer
{

class GLTexture;

class GLFrameBuffer
{
public:
	GLFrameBuffer();
	~GLFrameBuffer();

	bool initWithTextureParams(const std::vector<GLenum>& drawBuffers, const std::vector<GLenum>& pixelFormats, bool useRenderBufferForDepthStencil, const Size& size);

	GLuint getFrameBufferId() const { return _frameBufferId; }
	const std::vector<GLTexture*>& getTextures() const { return _textures; }

private:
	GLuint _frameBufferId;
	std::vector<GLTexture*> _textures;
};

} // namespace mgrrenderer
#endif
