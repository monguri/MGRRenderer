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

	bool initWithTextureParams(const std::vector<GLenum>& drawBuffers, const std::vector<GLenum>& pixelFormats, bool useRenderBufferForDepthStencil, bool isCubeMap, const Size& size); // �L���[�u�}�b�v�̂Ƃ���size.width�����g��Ȃ��̂Œ���
	//TODO:�Ƃ肠�����L���[�u�}�b�v�̓f�v�X�X�e���V���ɂ����g���ĂȂ��̂Ńf�v�X�X�e���V���̂��Ƃ̂ݍl�����Ă���
	void bindCubeMapFaceDepthStencil(GLenum face, size_t indexOfDrawBuffers);

	GLuint getFrameBufferId() const { return _frameBufferId; }
	const std::vector<GLTexture*>& getTextures() const { return _textures; }

private:
	bool _isCubeMap;
	GLuint _frameBufferId;
	std::vector<GLTexture*> _textures;
};

} // namespace mgrrenderer
#endif
