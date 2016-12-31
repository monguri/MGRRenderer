#include "GLTexture.h"

#if defined(MGRRENDERER_USE_OPENGL)
#include "Image.h"
#include "TextureUtility.h"
#include "GLProgram.h"

namespace mgrrenderer
{

struct PixelFormatInfo
{
	PixelFormatInfo(GLenum internalFormatVal, GLenum typeVal, int bitPerPixelVal, bool isCompressedVal, bool hasAlphaVal) :
		internalFormat(internalFormatVal),
		type(typeVal),
		bitPerPixel(bitPerPixelVal),
		isCompressed(isCompressedVal),
		hasAlpha(hasAlphaVal)
	{}

	GLenum internalFormat;
	GLenum type;
	int bitPerPixel;
	bool isCompressed;
	bool hasAlpha;
};

typedef std::map<TextureUtility::PixelFormat, PixelFormatInfo> PixelFormatInfoMap;

typedef PixelFormatInfoMap::value_type PixelFomatInfoMapValue;
static const PixelFomatInfoMapValue TexturePixelFormatInfoTable[] =
{
	PixelFomatInfoMapValue(TextureUtility::PixelFormat::RGBA8888, PixelFormatInfo(GL_RGBA, GL_UNSIGNED_BYTE, 32, false, true)),
	PixelFomatInfoMapValue(TextureUtility::PixelFormat::RGB888, PixelFormatInfo(GL_RGB, GL_UNSIGNED_BYTE, 24, false, false)),
	PixelFomatInfoMapValue(TextureUtility::PixelFormat::I8, PixelFormatInfo(GL_LUMINANCE, GL_UNSIGNED_BYTE, 8, false, false)),
	PixelFomatInfoMapValue(TextureUtility::PixelFormat::AI88, PixelFormatInfo(GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, 16, false, true)),
	PixelFomatInfoMapValue(TextureUtility::PixelFormat::RGBA4444, PixelFormatInfo(GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 16, false, true)),
	PixelFomatInfoMapValue(TextureUtility::PixelFormat::RGB5A1, PixelFormatInfo(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 16, false, true)),
};

// ����ȕ���static const��map���������ł���̂ˁB�B
static const PixelFormatInfoMap _pixelFormatInfoTable(
	TexturePixelFormatInfoTable,
	TexturePixelFormatInfoTable + sizeof(TexturePixelFormatInfoTable) / sizeof(TexturePixelFormatInfoTable[0])
);

GLTexture::GLTexture() : _textureId(0)
{
}

GLTexture::~GLTexture()
{
	if (_textureId != 0)
	{
		glDeleteTextures(1, &_textureId);
		_textureId = 0;
	}
}

bool GLTexture::initWithImage(const Image& image, TextureUtility::PixelFormat format)
{
	if (image.getRawData() == nullptr)
	{
		return false;
	}

	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	int imageWidth = image.getWidth();
	int imageHeight = image.getHeight();
	if (imageWidth > maxTextureSize || imageHeight > maxTextureSize)
	{	
		return false;
	}

	unsigned char* convertedData = nullptr;
	ssize_t convertedDataLen = 0;
	TextureUtility::PixelFormat pixelFormat = (format == TextureUtility::PixelFormat::NONE || format == TextureUtility::PixelFormat::AUTO) ? image.getPixelFormat() : format;
	TextureUtility::PixelFormat toFormat = convertDataToFormat(image.getRawData(), image.getRawDataLength(), image.getPixelFormat(), pixelFormat, &convertedData, &convertedDataLen);
	if (toFormat != pixelFormat)
	{
		goto ERR;
	}

	Logger::logAssert(_pixelFormatInfoTable.find(toFormat) != _pixelFormatInfoTable.end(), "�s�N�Z���t�H�[�}�b�g���e�[�u���ɂȂ��t�H�[�}�b�g���g�p���悤�Ƃ��Ă���");
	const PixelFormatInfo& info = _pixelFormatInfoTable.at(toFormat);

	unsigned int bytesPerRow = imageWidth * info.bitPerPixel / 8;
	if (bytesPerRow % 8 == 0)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
	}
	else if (bytesPerRow % 4 == 0)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}
	else if (bytesPerRow % 2 == 0)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	}
	else
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}

	if (_textureId != 0)
	{
		glDeleteTextures(1, &_textureId);
		_textureId = 0;
	}

	// TODO:�Ƃ肠����Mipmap�Ή��͂��Ȃ�
	glGenTextures(1, &_textureId);
	//glActiveTexture(GL_TEXTURE0); // �Ƃ肠����GL_TEXTURE0�����A�f�t�H���g��GL_TEXTURE0�Ȃ̂ŕK�v�Ƃ��Ȃ�
	glBindTexture(GL_TEXTURE_2D, _textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		goto ERR;
	}

	// TODO:���k�͖��l���B���k����glCompressedTexture2D���g���K�v������B�܂��Ainfo.internalFormat��info.format�̈Ⴂ�ɂ����l���̂��߁Ainfo.format�݂͐��ĂȂ�
	glTexImage2D(GL_TEXTURE_2D, 0, info.internalFormat, imageWidth, imageHeight, 0, info.internalFormat, info.type, (unsigned char*)convertedData);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		goto ERR;
	}
	// �e�N�X�`���̃f�[�^�͂����œ]�����������̂ŁAImage�̂��摜�f�[�^�͉������č\��Ȃ��BCCTextyureCache�ł�Image��Texture2D������������邵�ˁB����Ȃ�A�ʂɃN���X�ɂ��Ȃ��Ă�Utility�N���X�ł��悩�����Ǝv�����A�܂��f�[�^�𒆂ɓ���Ă�N���X�̕��������₷���͂���

	glBindTexture(GL_TEXTURE_2D, 0);

	// Image����f�[�^�����炤
	_contentSize = Size(imageWidth, imageHeight);
	return true;

ERR:
	if (convertedData != nullptr && convertedData != image.getRawData())
	{
		free(convertedData);
	}
	return false;
}

bool GLTexture::initDepthTexture(GLenum textureUnit, const Size& contentSize)
{
	_contentSize = contentSize;

	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	if (contentSize.width > maxTextureSize || contentSize.height > maxTextureSize)
	{	
		return false;
	}

	// �e�N�X�`��ID����
	glActiveTexture(textureUnit);
	glGenTextures(1, &_textureId);
	GLProgram::checkGLError();
	Logger::logAssert(_textureId != 0, "�f�v�X�e�N�X�`���������s");

	glBindTexture(GL_TEXTURE_2D, _textureId);
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		return false;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLfloat boarderColor[] = {1.0f, 0.0f, 0.0f, 0.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, boarderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	// �e�N�X�`������
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, static_cast<GLsizei>(contentSize.width), static_cast<GLsizei>(contentSize.height), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		glBindTexture(GL_TEXTURE_2D, 0);
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

	return true;
}

bool GLTexture::initDepthCubeMapTexture(GLenum textureUnit, float size)
{
	_contentSize = Size(size, size);

	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	if (size > maxTextureSize)
	{	
		return false;
	}

	// �e�N�X�`��ID����
	glActiveTexture(textureUnit);
	glEnable(GL_TEXTURE_CUBE_MAP); //TODO:�K�v���H
	glGenTextures(1, &_textureId);
	GLProgram::checkGLError();
	Logger::logAssert(_textureId != 0, "�f�v�X�e�N�X�`���������s");

	glBindTexture(GL_TEXTURE_CUBE_MAP, _textureId);
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		glActiveTexture(GL_TEXTURE0);
		return false;
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	GLfloat boarderColor[] = {1.0f, 0.0f, 0.0f, 0.0f};
	glTexParameterfv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BORDER_COLOR, boarderColor);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	// �L���[�u�}�b�v��6���̃e�N�X�`������
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_DEPTH_COMPONENT, static_cast<GLsizei>(size), static_cast<GLsizei>(size), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glActiveTexture(GL_TEXTURE0);
		return false;
	}

	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X + i, 0, GL_DEPTH_COMPONENT, static_cast<GLsizei>(size), static_cast<GLsizei>(size), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
		err = glGetError();
		if (err != GL_NO_ERROR)
		{
			Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			glActiveTexture(GL_TEXTURE0);
			return false;
		}
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glActiveTexture(GL_TEXTURE0);
	return true;
}

bool GLTexture::initRenderTexture(GLenum pixelFormat, const Size& contentSize)
{
	_contentSize = contentSize;
	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	if (contentSize.width > maxTextureSize || contentSize.height > maxTextureSize)
	{	
		return false;
	}

	// �e�N�X�`��ID����
	glGenTextures(1, &_textureId);
	//glActiveTexture(GL_TEXTURE0); // �Ƃ肠����GL_TEXTURE0�����A�f�t�H���g��GL_TEXTURE0�Ȃ̂ŕK�v�Ƃ��Ȃ�
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		return false;
	}
	if (_textureId == 0)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, _textureId);
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		return false;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat boarderColor[] = {1.0f, 0.0f, 0.0f, 0.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, boarderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	// �e�N�X�`������
	glTexImage2D(GL_TEXTURE_2D, 0, pixelFormat, static_cast<GLsizei>(contentSize.width), static_cast<GLsizei>(contentSize.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL�����ŃG���[���� glGetError()=%d", err);
		glBindTexture(GL_TEXTURE_2D, 0);
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}
} // namespace mgrrenderer
#endif
