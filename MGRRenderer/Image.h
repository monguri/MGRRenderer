#pragma once
#include <string>
//TODO:ssize_t�̂���
#include "BasicDataTypes.h"
#include "Texture.h"

namespace mgrrenderer
{

class Image final
{
public:
	enum class Format : int
	{
		UNKNOWN = -1,
		// ����png�����ɑΉ�
		PNG,
		TGA,
		NUM_FORMATS,
	};

	Image();
	~Image();
	bool initWithFilePath(const std::string& filePath);
	bool initWithImageData(const unsigned char* data, ssize_t dataLen);

	unsigned char* getData() const { return _data; };
	ssize_t getDataLength() const { return _dataLen; };
	int getWidth() const { return _width; }
	int getHeight() const { return _height; }
	bool getHasPremultipliedAlpha() const { return _hasPremultipliedAlpha; }
	Texture::PixelFormat getPixelFormat() const { return _pixelFormat; }

private:
	static const ssize_t PNG_SIGNATURE_SIZE = 8;
	unsigned char* _data;
	ssize_t _dataLen;
	int _width;
	int _height;
	Texture::PixelFormat _pixelFormat;
	bool _hasPremultipliedAlpha;

	Format detectFormat(const unsigned char * data, ssize_t dataLen);
	bool isPng(const unsigned char * data, ssize_t dataLen);
	bool initWithPngData(const unsigned char * data, ssize_t dataLen);
	bool initWithTgaData(const unsigned char * data, ssize_t dataLen);
	void premultiplyAlpha();
	inline static unsigned int premultiplyAlpha(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		// TODO:�Ȃ��o�C�g�I�[�_�[���t�ɂ���̂��H���Ԃ񃊃g���G���f�B�A���O��
		return (unsigned int)(
			r * (a + 1) >> 8 // *�ɂ���āA256*256���Ă�����̂�>>8��256�Ŋ���
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
