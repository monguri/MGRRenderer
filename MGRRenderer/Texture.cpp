#include "Texture.h"
#include "Image.h"


namespace mgrrenderer
{

static Texture::PixelFormat _defaultPixelFormat = Texture::PixelFormat::DEFAULT;

typedef Texture::PixelFormatInfoMap::value_type PixelFomatInfoMapValue;
static const PixelFomatInfoMapValue TexturePixelFormatInfoTable[] =
{
	PixelFomatInfoMapValue(Texture::PixelFormat::RGBA8888, Texture::PixelFormatInfo(GL_RGBA, GL_UNSIGNED_BYTE, 32, false, true)),
	PixelFomatInfoMapValue(Texture::PixelFormat::RGB888, Texture::PixelFormatInfo(GL_RGB, GL_UNSIGNED_BYTE, 24, false, false)),
	PixelFomatInfoMapValue(Texture::PixelFormat::I8, Texture::PixelFormatInfo(GL_LUMINANCE, GL_UNSIGNED_BYTE, 8, false, false)),
	PixelFomatInfoMapValue(Texture::PixelFormat::AI88, Texture::PixelFormatInfo(GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, 16, false, true)),
	PixelFomatInfoMapValue(Texture::PixelFormat::RGBA4444, Texture::PixelFormatInfo(GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 16, false, true)),
	PixelFomatInfoMapValue(Texture::PixelFormat::RGB5A1, Texture::PixelFormatInfo(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 16, false, true)),
};

// こんな風にstatic constなmapを初期化できるのね。。
const Texture::PixelFormatInfoMap Texture::_pixelFormatInfoTable(TexturePixelFormatInfoTable, TexturePixelFormatInfoTable + sizeof(TexturePixelFormatInfoTable) / sizeof(TexturePixelFormatInfoTable[0]));

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
	return initWithImage(image, _defaultPixelFormat);
}

bool Texture::initWithImage(const Image& image, PixelFormat format)
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
	PixelFormat pixelFormat = (format == PixelFormat::NONE || format == PixelFormat::AUTO) ? image.getPixelFormat() : format;
	PixelFormat toFormat = convertDataToFormat(image.getData(), image.getDataLength(), image.getPixelFormat(), pixelFormat, &convertedData, &convertedDataLen);
	if (toFormat != pixelFormat)
	{
		goto ERR;
	}

	Logger::logAssert(_pixelFormatInfoTable.find(toFormat) != _pixelFormatInfoTable.end(), "ピクセルフォーマット情報テーブルにないフォーマットを使用しようとしている");
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

	// TODO:圧縮は未考慮。圧縮時はglCompressedTexture2Dを使う必要がある。また、info.internalFormatとinfo.formatの違いにも未考慮のため、info.formatは設けてない
	glTexImage2D(GL_TEXTURE_2D, 0, info.internalFormat, imageWidth, imageHeight, 0, info.internalFormat, info.type, (unsigned char*)convertedData);
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
		Logger::log("unsupport convert for format %d to format %d", fromFormat, toFormat); // cocosでunsupportとなってたがなぜだ？
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
	case PixelFormat::RGBA4444:
		*outDataLen = dataLen * 2;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertI8ToRGBA4444(data, dataLen, *outData);
		break;
	default:
		if (toFormat != PixelFormat::AUTO || toFormat != PixelFormat::I8)
		{
			Logger::log("Can not convert image format PixelFormat::I8 to format ID:%d, we will use it's origin format PixelFormat::I8", toFormat);
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
	case PixelFormat::RGBA4444:
		*outDataLen = dataLen;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertAI88ToRGBA4444(data, dataLen, *outData);
		break;
	default:
		if (toFormat != PixelFormat::AUTO || toFormat != PixelFormat::AI88)
		{
			Logger::log("Can not convert image format PixelFormat::AI88 to format ID:%d, we will use it's origin format PixelFormat::AI88", toFormat);
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
	case PixelFormat::RGBA4444:
		*outDataLen = dataLen / 3 * 2;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertRGB888ToRGBA4444(data, dataLen, *outData);
		break;
	default:
		if (toFormat != PixelFormat::AUTO || toFormat != PixelFormat::RGB888)
		{
			Logger::log("Can not convert image format PixelFormat::RGB888 to format ID:%d, we will use it's origin format PixelFormat::RGB888", toFormat);
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
	case PixelFormat::RGBA4444:
		*outDataLen = dataLen / 2;
		*outData = (unsigned char*)malloc(sizeof(unsigned char) * (*outDataLen));
		convertRGBA8888ToRGBA4444(data, dataLen, *outData);
		break;
	default:
		if (toFormat != PixelFormat::AUTO || toFormat != PixelFormat::RGBA8888)
		{
			Logger::log("Can not convert image format PixelFormat::RGBA8888 to format ID:%d, we will use it's origin format PixelFormat::RGBA8888", toFormat);
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

void Texture::convertI8ToRGBA4444(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	unsigned short* out16 = (unsigned short*)outData;
	for (ssize_t i = 0; i < dataLen; ++i)
	{
		// 下位4ビットはカット
		*out16++ = (data[i] & 0x00F0) << 8 // R
			| (data[i] & 0x00F0) << 4 // G
			| (data[i] & 0x00F0) // B
			| 0x000F; // A
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

void Texture::convertAI88ToRGBA4444(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	unsigned short* out16 = (unsigned short*)outData;
	for (ssize_t i = 0; i < dataLen - 1; i += 2)
	{
		// 下位4ビットはカット
		*out16++ = (data[i] & 0x00F0) << 8 // R
			| (data[i] & 0x00F0) << 4 // G
			| (data[i] & 0x00F0) // B
			| (data[i + 1] & 0x00F0) >> 4; // A
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

void Texture::convertRGB888ToRGBA4444(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	unsigned short* out16 = (unsigned short*)outData;
	for (ssize_t i = 0; i < dataLen - 2; i += 3)
	{
		// 下位4ビットはカット
		*out16++ = (data[i] & 0x00F0) << 8 // R
			| (data[i + 1] & 0x00F0) << 4 // G
			| (data[i + 2] & 0x00F0) // B
			| 0x000F; // A
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

void Texture::convertRGBA8888ToRGBA4444(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	unsigned short* out16 = (unsigned short*)outData;
	for (ssize_t i = 0; i < dataLen - 3; i += 4)
	{
		// 下位4ビットはカット
		*out16++ = (data[i] & 0x00F0) << 8 // R
			| (data[i + 1] & 0x00F0) << 4 // G
			| (data[i + 2] & 0x00F0) // B
			| (data[i + 3] & 0x00F0) >> 4; // A
	}
}

void Texture::setDefaultPixelFormat(PixelFormat format)
{
	_defaultPixelFormat = format;
}

} // namespace mgrrenderer
