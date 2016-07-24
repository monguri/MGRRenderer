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

// こんな風にstatic constなmapを初期化できるのね。。
static const PixelFormatInfoMap _pixelFormatInfoTable(
	TexturePixelFormatInfoTable,
	TexturePixelFormatInfoTable + sizeof(TexturePixelFormatInfoTable) / sizeof(TexturePixelFormatInfoTable[0])
);

GLTexture::GLTexture() : _textureId(0), _frameBufferId(0)
{
}

GLTexture::~GLTexture()
{
	if (_frameBufferId != 0)
	{
		glDeleteFramebuffers(1, &_frameBufferId);
		_frameBufferId = 0;
	}

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
	return true;

ERR:
	if (convertedData != nullptr && convertedData != image.getRawData())
	{
		free(convertedData);
	}
	return false;
}

bool GLTexture::initDepthTexture(const Size& contentSize)
{
	_contentSize = contentSize;

	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	if (contentSize.width > maxTextureSize || contentSize.height > maxTextureSize)
	{	
		return false;
	}

	// テクスチャID生成
	glGenTextures(1, &_textureId);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	Logger::logAssert(_textureId != 0, "デプステクスチャ生成失敗");

	glBindTexture(GL_TEXTURE_2D, _textureId);
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL処理でエラー発生 glGetError()=%d", err);
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

	// テクスチャ生成
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, contentSize.width, contentSize.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL処理でエラー発生 glGetError()=%d", err);
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	// デプステクスチャに描画するためのフレームバッファ作成
	glGenFramebuffers(1, &_frameBufferId);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL処理でエラー発生 glGetError()=%d", err);
		return false;
	}
	if (_frameBufferId == 0)
	{
		Logger::logAssert(false, "フレームバッファ生成失敗");
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferId);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL処理でエラー発生 glGetError()=%d", err);
		return false;
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _textureId, 0);
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL処理でエラー発生 glGetError()=%d", err);
		return false;
	}

	GLenum drawBuffers[] = {GL_NONE};
	glDrawBuffers(1, drawBuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		Logger::logAssert(false, "デプスシャドウ用のフレームバッファが完成してない");
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // デフォルトのフレームバッファに戻す
	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		Logger::logAssert(false, "OpenGL処理でエラー発生 glGetError()=%d", err);
		return false;
	}

	return true;
}
} // namespace mgrrenderer
#endif
