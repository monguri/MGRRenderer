#include "GLTexture.h"

#if defined(MGRRENDERER_USE_OPENGL)
#include "Image.h"
#include "TextureUtility.h"

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
	if (image.getData() == nullptr)
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
	TextureUtility::PixelFormat toFormat = convertDataToFormat(image.getData(), image.getDataLength(), image.getPixelFormat(), pixelFormat, &convertedData, &convertedDataLen);
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
	glActiveTexture(GL_TEXTURE0); // TODO:�Ƃ肠����GL_TEXTURE0
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

	// Image����f�[�^�����炤
	_contentSize = Size(imageWidth, imageHeight);
	return true;

ERR:
	if (convertedData != nullptr && convertedData != image.getData())
	{
		free(convertedData);
	}
	return false;
}

bool GLTexture::initWithTexture(GLuint textureId, const Size& contentSize, TextureUtility::PixelFormat format)
{
	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	if (contentSize.width > maxTextureSize || contentSize.height > maxTextureSize)
	{	
		return false;
	}

	const PixelFormatInfo& info = _pixelFormatInfoTable.at(format);

	unsigned int bytesPerRow = contentSize.width * info.bitPerPixel / 8;
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
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		return false;
	}

	_textureId = textureId;
	_contentSize = contentSize;

	return true;
}

} // namespace mgrrenderer
#endif