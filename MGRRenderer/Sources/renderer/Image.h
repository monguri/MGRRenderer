#pragma once
#include <string>

namespace mgrrenderer
{

namespace TextureUtility
{
enum class PixelFormat;
}

class Image final
{
public:
	enum class FileFormat : int
	{
		UNKNOWN = -1,
		PNG,
		TGA,
		NUM_FORMATS,
	};

	Image();
	~Image();
	bool initWithFilePath(const std::string& filePath);
	bool initWithImageData(const unsigned char* data, size_t dataLen);

	unsigned char* getData() const { return _data; };
	size_t getDataLength() const { return _dataLen; };
	unsigned char* getRawData() const { return _rawData; };
	size_t getRawDataLength() const { return _rawDataLen; };
	unsigned int getWidth() const { return _width; }
	unsigned int getHeight() const { return _height; }
	bool getHasPremultipliedAlpha() const { return _hasPremultipliedAlpha; }
	TextureUtility::PixelFormat getPixelFormat() const { return _pixelFormat; }
	FileFormat getFileFormat() const { return _fileFormat; }

private:
	static const size_t PNG_SIGNATURE_SIZE = 8;
	unsigned char* _data;
	size_t _dataLen;
	// ヘッダ情報や圧縮のない生の画像データ
	unsigned char* _rawData;
	size_t _rawDataLen;
	unsigned int _width;
	unsigned int _height;
	TextureUtility::PixelFormat _pixelFormat;
	bool _hasPremultipliedAlpha;
	FileFormat _fileFormat;

	FileFormat detectFileFormat(const unsigned char * data, size_t dataLen);
	bool isPng(const unsigned char * data, size_t dataLen);
	bool initWithPngData(const unsigned char * data, size_t dataLen);
	bool initWithTgaData(const unsigned char * data, size_t dataLen);
	void premultiplyAlpha();
	inline static unsigned int premultiplyAlpha(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		// TODO:なぜバイトオーダーを逆にするのか？たぶんリトルエンディアン前提
		return (unsigned int)(
			r * (a + 1) >> 8 // *によって、256*256しているものを>>8で256で割る
			| (g * (a + 1) >> 8) << 8
			| (b * (a + 1) >> 8) << 16
			| a << 24
			);
	}
#define CC_RGB_PREMULTIPLY_ALPHA(vr, vg, vb, va) \
    (unsigned)(((unsigned)((unsigned char)(vr) * ((unsigned char)(va) + 1)) >> 8) | \
    ((unsigned)((unsigned char)(vg) * ((unsigned char)(va) + 1) >> 8) << 8) | \
    ((unsigned)((unsigned char)(vb) * ((unsigned char)(va) + 1) >> 8) << 16) | \
    ((unsigned)(unsigned char)(va) << 24))

};

} // namespace mgrrenderer
