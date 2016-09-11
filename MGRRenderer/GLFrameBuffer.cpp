#include "GLFrameBuffer.h"
#include "Logger.h"
#include "GLTexture.h"

namespace mgrrenderer
{


GLFrameBuffer::GLFrameBuffer() : _frameBufferId(0)
{
}

GLFrameBuffer::~GLFrameBuffer()
{
	for (GLTexture* texture : _textures)
	{
		delete texture;
	}

	if (_frameBufferId != 0)
	{
		glDeleteFramebuffers(1, &_frameBufferId);
		_frameBufferId = 0;
	}
}

bool GLFrameBuffer::initWithTextureParams(std::vector<GLenum> drawBuffers, std::vector<GLenum> pixelFormats, const Size& size)
{
	Logger::logAssert(drawBuffers.size() == pixelFormats.size(), "�����̗v�f�����s��v�B");

	// �����_�[�e�N�X�`���ɕ`�悷�邽�߂̃t���[���o�b�t�@�쐬
	glGenFramebuffers(1, &_frameBufferId);
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		return false;
	}
	if (_frameBufferId == 0)
	{
		Logger::logAssert(false, "�t���[���o�b�t�@�������s");
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferId);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		return false;
	}

	size_t numTexture = drawBuffers.size();
	for (size_t i = 0; i < numTexture; i++)
	{

		switch (drawBuffers[i])
		{
		case GL_NONE: // �f�v�X
		case GL_DEPTH_ATTACHMENT: // �f�v�X
		{
#if 0 // �f�v�X�e�N�X�`���łȂ������_�[�o�b�t�@���g���`��
			glGenRenderbuffers(1, &textureId);
			err = glGetError();
			if (err != GL_NO_ERROR)
			{
				Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
				return false;
			}
			if (textureId == 0)
			{
				Logger::logAssert(false, "�t���[���o�b�t�@�������s");
				return false;
			}

			glBindRenderbuffer(GL_RENDERBUFFER, textureId);
			err = glGetError();
			if (err != GL_NO_ERROR)
			{
				Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
				return false;
			}
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.width, size.height);
			err = glGetError();
			if (err != GL_NO_ERROR)
			{
				Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
				return false;
			}

			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, textureId);
			err = glGetError();
			if (err != GL_NO_ERROR)
			{
				Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
				return false;
			}
#endif
			GLTexture* texture = new GLTexture();
			texture->initDepthTexture(GL_TEXTURE0 + i, size);
			_textures.push_back(texture);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->getTextureId(), 0);
			err = glGetError();
			if (err != GL_NO_ERROR)
			{
				Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
				return false;
			}
		}
			break;
		case GL_COLOR_ATTACHMENT0:
		case GL_COLOR_ATTACHMENT1:
		case GL_COLOR_ATTACHMENT2:
		{
			GLTexture* texture = new GLTexture();
			texture->initRenderTexture(GL_TEXTURE0 + i, pixelFormats[i], size);
			_textures.push_back(texture);

			glFramebufferTexture2D(GL_FRAMEBUFFER, drawBuffers[i], GL_TEXTURE_2D, texture->getTextureId(), 0);
			err = glGetError();
			if (err != GL_NO_ERROR)
			{
				Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
				return false;
			}
		}
			break;
		default:
			Logger::logAssert(false, "�z�肵�ĂȂ�drawBuffer�^�C�v�����͂��ꂽ�B");
			break;
		}

	}

	glDrawBuffers(drawBuffers.size(), drawBuffers.data());

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		Logger::logAssert(false, "�����_�[�e�N�X�`���p�̃t���[���o�b�t�@���������ĂȂ�");
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // �f�t�H���g�̃t���[���o�b�t�@�ɖ߂�
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		return false;
	}

	return true;
}

} // namespace mgrrenderer
