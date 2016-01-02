#include "Texture.h"
#include "Image.h"


namespace mgrrenderer
{

Texture::Texture() :
_textureId(0),
_contentSize(Size(0, 0)),
_pixelWidth(0),
_pixelHeight(0),
_hasPremultipliedAlpha(true)
{
}

Texture::~Texture()
{
	if (_textureId != 0)
	{
		glDeleteTextures(1, &_textureId);
		_textureId = 0;
	}
}

bool Texture::initWithImage(const Image& image)
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
	// TODO:今はレンダーバッファをPixelFormat::RGBA8888フォーマットにしか対応しないのでし画像もRGBA8888に変換
	PixelFormat toFormat = convertDataToFormat(image.getData(), image.getDataLength(), image.getPixelFormat(), PixelFormat::RGBA8888, &convertedData, &convertedDataLen);
	if (toFormat != PixelFormat::RGBA8888)
	{
		goto ERR;
	}

	//PixelFormatInfo(GLenum anInternalFormat, GLenum aFormat, GLenum aType, int aBpp, bool aCompressed, bool anAlpha)
	//	: internalFormat(anInternalFormat)
	//	, format(aFormat)
	//	, type(aType)
	//	, bpp(aBpp)
	//	, compressed(aCompressed)
	//	, alpha(anAlpha)
	//{}

	//PixelFormatInfoMapValue(Texture2D::PixelFormat::RGBA8888, Texture2D::PixelFormatInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 32, false, true)),

	// TODO:この処理いるか？
	static const int BITS_PER_PIXEL = 32; // RGBA8888固定
	unsigned int bytesPerRow = imageWidth * BITS_PER_PIXEL / 8;
	if (bytesPerRow % 8 == 0)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
	}
	else if (bytesPerRow % 4 == 0)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	if (_textureId != 0)
	{
		glDeleteTextures(1, &_textureId);
		_textureId = 0;
	}

	// TODO:とりあえずMipmap対応はしない
	glGenTextures(1, &_textureId);
	glActiveTexture(GL_TEXTURE0); // TODO:とりあえずGL_TEXTURE0
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

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)convertedData);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		goto ERR;
	}
	// テクスチャのデータはここで転送完了したので、Imageのもつ画像データは解放されて構わない。CCTextyureCacheでもImageはTexture2D作ったら解放するしね。それなら、別にクラスにしなくてもUtilityクラスでもよかったと思うが、まあデータを中に内包してるクラスの方が扱いやすくはある

	// Imageからデータをもらう
	_contentSize = Size(imageWidth, imageHeight);
	_pixelWidth = imageWidth;
	_pixelHeight = imageHeight;
	_hasPremultipliedAlpha = image.getHasPremultipliedAlpha();


	// 以下はSprite側でやろう
	//setGLProgram
	// Texture2Dでやってた処理
	//setGLProgram(GLProgramCache::getInstance()->getGLProgram(GLProgram::SHADER_NAME_POSITION_TEXTURE));
	// Spriteでやってた処理
	//setGLProgramState(GLProgramState::getOrCreateWithGLProgramName(GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR_NO_MVP));

	//_glData = createOpenGLProgram(
	return true;

ERR:
	if (convertedData != nullptr && convertedData != image.getData())
	{
		free(convertedData);
	}

	return false;
}

Texture::PixelFormat Texture::convertDataToFormat(const unsigned char* data, ssize_t dataLen, PixelFormat fromFormat, PixelFormat toFormat, unsigned char** outData, ssize_t* outDataLen)
{
	if (fromFormat == toFormat || fromFormat == PixelFormat::AUTO)
	{
		*outData = (unsigned char*)data;
		*outDataLen = dataLen;
		return fromFormat;
	}

	switch (fromFormat)
	{
	case PixelFormat::I8:
		return convertI8ToFormat(data, dataLen, toFormat, outData, outDataLen);
	case PixelFormat::AI88:
		return convertAI88ToFormat(data, dataLen, toFormat, outData, outDataLen);
	case PixelFormat::RGBA8888:
		return convertRGBA8888ToFormat(data, dataLen, toFormat, outData, outDataLen);
	case PixelFormat::RGB888:
		return convertRGB888ToFormat(data, dataLen, toFormat, outData, outDataLen);
	default:
		printf("unsupport convert for format %d to format %d", fromFormat, toFormat); // cocosでunsupportとなってたがなぜだ？
		*outData = (unsigned char*)data;
		*outDataLen = dataLen;
		return fromFormat;
	}
}

Texture::PixelFormat Texture::convertI8ToFormat(const unsigned char* data, ssize_t dataLen, PixelFormat toFormat, unsigned char** outData, ssize_t* outDataLen)
{
	switch (toFormat)
	{
	case PixelFormat::RGBA8888:
		*outDataLen = dataLen * 4;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertI8ToRGBA8888(data, dataLen, *outData);
		break;
	case PixelFormat::RGB888:
		*outDataLen = dataLen * 3;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertI8ToRGB888(data, dataLen, *outData);
		break;
	case PixelFormat::AI88:
		*outDataLen = dataLen * 2;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertI8ToAI88(data, dataLen, *outData);
		break;
	default:
		if (toFormat != PixelFormat::AUTO || toFormat != PixelFormat::I8)
		{
			printf("Can not convert image format PixelFormat::I8 to format ID:%d, we will use it's origin format PixelFormat::I8", toFormat);
		}
		*outData = (unsigned char*)data;
		*outDataLen = dataLen;
		return PixelFormat::I8;
	}

	return toFormat;
}

Texture::PixelFormat Texture::convertAI88ToFormat(const unsigned char* data, ssize_t dataLen, PixelFormat toFormat, unsigned char** outData, ssize_t* outDataLen)
{
	switch (toFormat)
	{
	case PixelFormat::RGBA8888:
		*outDataLen = dataLen * 2;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertAI88ToRGBA8888(data, dataLen, *outData);
		break;
	case PixelFormat::RGB888:
		*outDataLen = dataLen / 2 * 3;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertAI88ToRGB888(data, dataLen, *outData);
		break;
	case PixelFormat::I8:
		*outDataLen = dataLen / 2;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertAI88ToI8(data, dataLen, *outData);
		break;
	default:
		if (toFormat != PixelFormat::AUTO || toFormat != PixelFormat::AI88)
		{
			printf("Can not convert image format PixelFormat::AI88 to format ID:%d, we will use it's origin format PixelFormat::AI88", toFormat);
		}
		*outData = (unsigned char*)data;
		*outDataLen = dataLen;
		return PixelFormat::AI88;
	}

	return toFormat;
}

Texture::PixelFormat Texture::convertRGB888ToFormat(const unsigned char* data, ssize_t dataLen, PixelFormat toFormat, unsigned char** outData, ssize_t* outDataLen)
{
	switch (toFormat)
	{
	case PixelFormat::RGBA8888:
		*outDataLen = dataLen / 3 * 4;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertRGB888ToRGBA8888(data, dataLen, *outData);
		break;
	case PixelFormat::I8:
		*outDataLen = dataLen / 3;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertRGB888ToI8(data, dataLen, *outData);
		break;
	case PixelFormat::AI88:
		*outDataLen = dataLen / 3 * 2;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertRGB888ToAI88(data, dataLen, *outData);
		break;
	default:
		if (toFormat != PixelFormat::AUTO || toFormat != PixelFormat::RGB888)
		{
			printf("Can not convert image format PixelFormat::RGB888 to format ID:%d, we will use it's origin format PixelFormat::RGB888", toFormat);
		}
		*outData = (unsigned char*)data;
		*outDataLen = dataLen;
		return PixelFormat::RGB888;
	}

	return toFormat;
}

Texture::PixelFormat Texture::convertRGBA8888ToFormat(const unsigned char* data, ssize_t dataLen, PixelFormat toFormat, unsigned char** outData, ssize_t* outDataLen)
{
	switch (toFormat)
	{
	case PixelFormat::RGB888:
		*outDataLen = dataLen / 4 * 3;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertRGBA8888ToRGB888(data, dataLen, *outData);
		break;
	case PixelFormat::I8:
		*outDataLen = dataLen / 4;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertRGBA8888ToI8(data, dataLen, *outData);
		break;
	case PixelFormat::AI88:
		*outDataLen = dataLen / 2;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertRGBA8888ToAI88(data, dataLen, *outData);
		break;
	default:
		if (toFormat != PixelFormat::AUTO || toFormat != PixelFormat::RGBA8888)
		{
			printf("Can not convert image format PixelFormat::RGBA8888 to format ID:%d, we will use it's origin format PixelFormat::RGBA8888", toFormat);
		}
		*outData = (unsigned char*)data;
		*outDataLen = dataLen;
		return PixelFormat::RGBA8888;
	}

	return toFormat;
}

void Texture::convertI8ToRGBA8888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0; i < dataLen; ++i)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i]; // G
		*outData++ = data[i]; // B
		*outData++ = 0xFF; // A
	}
}

void Texture::convertI8ToRGB888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0; i < dataLen; ++i)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i]; // G
		*outData++ = data[i]; // B
	}
}

void Texture::convertI8ToAI88(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	unsigned short* outData16 = (unsigned short*)outData;
	for (ssize_t i = 0; i < dataLen; ++i)
	{
		*outData16++ = 0xFF00 // A
			| data[i]; // I
	}
}

void Texture::convertAI88ToRGBA8888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 1; i < l; i += 2)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i]; // G
		*outData++ = data[i]; // B
		*outData++ = data[i + 1]; // A
	}
}

void Texture::convertAI88ToRGB888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 1; i < l; i += 2)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i]; // G
		*outData++ = data[i]; // B
	}
}

void Texture::convertAI88ToI8(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 1; i < l; i += 2)
	{
		*outData++ = data[i]; // R
	}
}

void Texture::convertRGB888ToRGBA8888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 2; i < l; i += 3)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i + 1]; // G
		*outData++ = data[i + 2]; // B
		*outData++ = 0xFF; // A
	}
}

void Texture::convertRGB888ToI8(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 2; i < l; i += 3)
	{
		*outData++ = (data[i] * 299 + data[i + 1] * 587 + data[i + 2] * 114 + 500) / 1000; // I = (R * 299 + G * 587 + B * 114 + 500) / 1000 // TODO:どういう数式やろかこれ。。
	}
}

void Texture::convertRGB888ToAI88(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 2; i < l; i += 3)
	{
		*outData++ = (data[i] * 299 + data[i + 1] * 587 + data[i + 2] * 114 + 500) / 1000; // I = (R * 299 + G * 587 + B * 114 + 500) / 1000 // TODO:どういう数式やろかこれ。。
		*outData++ = 0xFF;
	}
}

void Texture::convertRGBA8888ToRGB888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 3; i < l; i += 4)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i + 1]; // G
		*outData++ = data[i + 2]; // B
	}
}

void Texture::convertRGBA8888ToI8(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 3; i < l; i += 4)
	{
		*outData++ = (data[i] * 299 + data[i + 1] * 587 + data[i + 2] * 114 + 500) / 1000; // I = (R * 299 + G * 587 + B * 114 + 500) / 1000 // TODO:どういう数式やろかこれ。。
	}
}

void Texture::convertRGBA8888ToAI88(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 3; i < l; i += 4)
	{
		*outData++ = (data[i] * 299 + data[i + 1] * 587 + data[i + 2] * 114 + 500) / 1000; // I = (R * 299 + G * 587 + B * 114 + 500) / 1000 // TODO:どういう数式やろかこれ。。
		*outData++ = data[i + 3];
	}
}

} // namespace mgrrenderer
