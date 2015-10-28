#include "Image.h"
#include "FileUtility.h"
#include <assert.h>

extern "C"
{
#include "png/include/png.h"
}

namespace mgrrenderer
{

typedef struct
{
	const unsigned char* data;
	ssize_t size;
	int offset;
} ImageSource;

Image::Image() :
_data(nullptr),
_dataLen(0),
_width(0),
_height(0),
_pixelFormat(Texture::PixelFormat::NONE),
_hasPremultipliedAlpha(true)
{
}

Image::~Image()
{
	if (_data != nullptr)
	{
		free(_data);
	}
}

bool Image::initWithFilePath(const std::string& filePath)
{
	FileUtility* fileUtil = FileUtility::getInstance();
	const std::string& fullPath = fileUtil->getFullPathForFileName(filePath);
	ssize_t fileSize = 0;
	unsigned char* fileData = fileUtil->getFileData(fullPath, &fileSize);
	// libpng、libjpegによってrawデータに変換

	// TODO:unzipってlibpngに必要なのかな？→必要ない
	Format format = detectFormat(fileData, fileSize);
	bool isSucceeded = false;
	switch (format)
	{
	case Format::PNG:
		isSucceeded = initWithPngData(fileData, fileSize);
		break;
	default:
		break;
	}

	return isSucceeded;
}

Image::Format Image::detectFormat(const unsigned char * data, ssize_t dataLen)
{
	if (isPng(data, dataLen))
	{
		return Image::Format::PNG;
	}
	
	return Image::Format::UNKNOWN;
}

bool Image::isPng(const unsigned char * data, ssize_t dataLen)
{
	if (dataLen < PNG_SIGNATURE_SIZE)
	{
		return false;
	}

	static const unsigned char PNG_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

	return memcmp(data, PNG_SIGNATURE, sizeof(PNG_SIGNATURE)) == 0;
}

bool Image::initWithPngData(const unsigned char * data, ssize_t dataLen)
{
	if (dataLen < PNG_SIGNATURE_SIZE)
	{
		return false;
	}

	// ヘッダ部分をチェック
	png_byte header[PNG_SIGNATURE_SIZE] = {0};
	memcpy(header, data, PNG_SIGNATURE_SIZE);
	if (png_sig_cmp(header, 0, PNG_SIGNATURE_SIZE) != 0)
	{
		return false;
	}

	// pngデータ構造体を作成
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (png == nullptr)
	{
		return false;
	}

	// png情報構造体を作成
	png_infop info = png_create_info_struct(png);
	if (info == nullptr)
	{
		return false;
	}

	// よくわからん
	if (setjmp(png_jmpbuf(png))) // 戻り値の型がわからん。。
	{
		return false;
	}

	ImageSource imageSource;
	imageSource.data = data;
	imageSource.size = dataLen;
	imageSource.offset = 0;

	// pngデータ構造体にロード
	png_set_read_fn(png, &imageSource, [](png_structp png, png_bytep data, png_size_t length) { // なぜか同期処理なのにコールバックを与える形
		ImageSource* imageSource = (ImageSource*)png_get_io_ptr(png);

		if ((imageSource->offset + length) <= imageSource->size)
		{
			memcpy(data, imageSource->data + imageSource->offset, length);
			imageSource->offset += length;
		}
		else
		{
			png_error(png, "png read callback failed.");
		}
	});

	// png情報構造体にロード
	png_read_info(png, info);

	_width = png_get_image_width(png, info);
	_height = png_get_image_height(png, info);

	// フォーマットに依存した整形処理
	png_byte bitDepth = png_get_bit_depth(png, info);
	png_uint_32 colorType = png_get_color_type(png, info);

	if (colorType == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(png);
	}

	if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
	{
		bitDepth = 8;
		png_set_expand_gray_1_2_4_to_8(png);
	}

	if (png_get_valid(png, info, PNG_INFO_tRNS))
	{
		png_set_tRNS_to_alpha(png);
	}

	if (bitDepth == 16)
	{
		png_set_strip_16(png);
	}

	if (bitDepth < 8)
	{
		png_set_packing(png);
	}

	png_read_update_info(png, info);
	bitDepth = png_get_bit_depth(png, info);
	colorType = png_get_color_type(png, info);

	switch (colorType)
	{
	case PNG_COLOR_TYPE_GRAY:
		_pixelFormat = Texture::PixelFormat::I8;
		break;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		_pixelFormat = Texture::PixelFormat::AI88;
		break;
	case PNG_COLOR_TYPE_RGB:
		_pixelFormat = Texture::PixelFormat::RGB888;
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		_pixelFormat = Texture::PixelFormat::RGBA8888;
		break;
	default:
		break;
	}

	// 一行一行ロードしてメンバに格納
	png_bytep* rowPointers = static_cast<png_bytep*>(malloc(sizeof(png_bytep) * _height));
	png_size_t rowSize = png_get_rowbytes(png, info);

	_dataLen = rowSize * _height;
	_data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
	if (_data == nullptr)
	{
		if (rowPointers == nullptr)
		{
			free(rowPointers);
		}

		return false;
	}

	for (unsigned short i = 0; i < _height; ++i)
	{
		rowPointers[i] = _data + i * rowSize;
	}
	png_read_image(png, rowPointers);

	png_read_end(png, nullptr);

	if (colorType == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		premultiplyAlpha();
	}
	else
	{
		_hasPremultipliedAlpha = false;
	}

	if (rowPointers != nullptr)
	{
		free(rowPointers);
	}

	return true;
}

void Image::premultiplyAlpha()
{
	assert(_pixelFormat == Texture::PixelFormat::RGBA8888);

	unsigned int* fourBytes = (unsigned int*)_data;
	for (int i = 0; i < _width * _height; i++)
	{
		unsigned char* p = _data + i * 4;
		// TODO:これくらいのことはGPUでやってもいいんじゃないかなあ。まあ、わざわざ転送するのもなんだけど。
		fourBytes[i] = premultiplyAlpha(p[0], p[1], p[2], p[3]);
	}

	_hasPremultipliedAlpha = true;
}

} // namespace mgrrenderer
