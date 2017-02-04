#include "Image.h"
#include "utility/FileUtility.h"
#include "utility/Logger.h"
#include "TextureUtility.h"

extern "C"
{
#include "png/include/png.h"
}

namespace mgrrenderer
{

typedef struct
{
	const unsigned char* data;
	size_t size;
	int offset;
} ImageSource;

Image::Image() :
_data(nullptr),
_dataLen(0),
_rawData(nullptr),
_rawDataLen(0),
_width(0),
_height(0),
#if defined(MGRRENDERER_USE_OPENGL)
_pixelFormat(TextureUtility::PixelFormat::NONE),
#endif
_hasPremultipliedAlpha(true),
_fileFormat(FileFormat::UNKNOWN)
{
}

Image::~Image()
{
	if (_rawData != nullptr)
	{
		free(_rawData);
	}

	if (_data != nullptr)
	{
		free(_data);
	}
}

bool Image::initWithFilePath(const std::string& filePath)
{
	FileUtility* fileUtil = FileUtility::getInstance();
	const std::string& fullPath = fileUtil->getFullPathForFileName(filePath);
	size_t fileSize = 0;
	unsigned char* fileData = fileUtil->getFileData(fullPath, &fileSize);
	// libpng�Alibjpeg�ɂ����raw�f�[�^�ɕϊ�

	// _data, _dataLen��initWithImageData���Ŋi�[���Ă���
	bool isSucceeded = initWithImageData(fileData, fileSize);
	free(fileData);
	return isSucceeded;
}

bool Image::initWithImageData(const unsigned char* data, size_t dataLen)
{
	// libpng�Alibjpeg�ɂ����raw�f�[�^�ɕϊ�
	_data = static_cast<unsigned char*>(malloc(dataLen));
	memcpy(_data, data, dataLen);
	_dataLen = dataLen;

	_fileFormat = detectFileFormat(data, dataLen);
	bool isSucceeded = false;
	switch (_fileFormat)
	{
	case FileFormat::PNG:
		isSucceeded = initWithPngData(data, dataLen);
		break;
	default:
		// �t�H�[�}�b�g�̂킩��Ȃ����̂�tga�Ƃ��ēǂ�ł݂�Btga�̓t�@�C������tga�`���ł��邱�Ƃ��������̂��K����������킯�łȂ�
		_fileFormat = FileFormat::TGA;
		isSucceeded = initWithTgaData(data, dataLen);
		//TODO: _fileData�̃w�b�_�ƃt�b�^��������������_rawData�ɃR�s�[����`���Ȃ̂Ńw�b�_�ƃt�b�^�͉�����Ȃ���
		if (!isSucceeded)
		{
			Logger::log("unsupport image format.");
		}
		break;
	}

	return isSucceeded;
}

Image::FileFormat Image::detectFileFormat(const unsigned char * data, size_t dataLen)
{
	if (isPng(data, dataLen))
	{
		return Image::FileFormat::PNG;
	}
	
	return Image::FileFormat::UNKNOWN;
}

bool Image::isPng(const unsigned char * data, size_t dataLen)
{
	if (dataLen < PNG_SIGNATURE_SIZE)
	{
		return false;
	}

	static const unsigned char PNG_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

	return memcmp(data, PNG_SIGNATURE, sizeof(PNG_SIGNATURE)) == 0;
}

bool Image::initWithPngData(const unsigned char * data, size_t dataLen)
{
	if (dataLen < PNG_SIGNATURE_SIZE)
	{
		return false;
	}

	// �w�b�_�������`�F�b�N
	png_byte header[PNG_SIGNATURE_SIZE] = {0};
	memcpy(header, data, PNG_SIGNATURE_SIZE);
	if (png_sig_cmp(header, 0, PNG_SIGNATURE_SIZE) != 0)
	{
		return false;
	}

	// png�f�[�^�\���̂��쐬
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (png == nullptr)
	{
		return false;
	}

	// png���\���̂��쐬
	png_infop info = png_create_info_struct(png);
	if (info == nullptr)
	{
		return false;
	}

	#pragma warning(disable:4611) // �x����Level4�ȏ㌟�o�ŏo��̂ŁA����̂��߂ɗ}������
	// Warning	C4611	interaction between '_setjmp' and C++ object destruction is non-portable
	// �悭�킩���
	if (setjmp(png_jmpbuf(png))) // �߂�l�̌^���킩���B�B
	{
		return false;
	}

	ImageSource imageSource;
	imageSource.data = data;
	imageSource.size = dataLen;
	imageSource.offset = 0;

	// png�f�[�^�\���̂Ƀ��[�h
	png_set_read_fn(png, &imageSource, [](png_structp png, png_bytep data, png_size_t length) { // �Ȃ������������Ȃ̂ɃR�[���o�b�N��^����`
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

	// png���\���̂Ƀ��[�h
	png_read_info(png, info);

	_width = png_get_image_width(png, info);
	_height = png_get_image_height(png, info);

	// �t�H�[�}�b�g�Ɉˑ��������`����
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

#if defined(MGRRENDERER_USE_OPENGL)
	switch (colorType)
	{
	case PNG_COLOR_TYPE_GRAY:
		_pixelFormat = TextureUtility::PixelFormat::I8;
		break;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		_pixelFormat = TextureUtility::PixelFormat::AI88;
		break;
	case PNG_COLOR_TYPE_RGB:
		_pixelFormat = TextureUtility::PixelFormat::RGB888;
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		_pixelFormat = TextureUtility::PixelFormat::RGBA8888;
		break;
	default:
		break;
	}
#endif

	// ��s��s���[�h���ă����o�Ɋi�[
	png_bytep* rowPointers = static_cast<png_bytep*>(malloc(sizeof(png_bytep) * _height));
	png_size_t rowSize = png_get_rowbytes(png, info);

	_rawDataLen = rowSize * _height;
	_rawData = static_cast<unsigned char*>(malloc(_rawDataLen * sizeof(unsigned char)));
	if (_rawData == nullptr)
	{
		if (rowPointers == nullptr)
		{
			free(rowPointers);
		}

		return false;
	}

	for (unsigned short i = 0; i < _height; ++i)
	{
		rowPointers[i] = _rawData + i * rowSize;
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

bool Image::initWithTgaData(const unsigned char* data, size_t dataLen)
{
	//
	// �w�b�_�����̃��[�h
	//
	struct TgaHeader {
		int status;
		unsigned char type;
		unsigned char pixelDepth;
		signed short width;
		signed short height;
		int flipped;
	};

	TgaHeader header;

	if (dataLen < 6 * sizeof(unsigned char) + 6 * sizeof(signed short))
	{
		// �f�[�^�������w�b�_���̋K��̒�����菬������΃G���[
		// TODO:��H�ނ���o�b�t�@�������œn�����A���ŕK�v�Ȃ����̃��������m�ۂ��ďo�͈����ŊO�ɓn���΂����̂ł́H�T�C�Y���o�͈����ŁB
		return false;
	}
	size_t step = 2 * sizeof(unsigned char); // ID field length and color map flag
	memcpy(&header.type, data + step, sizeof(unsigned char));
	step += sizeof(unsigned char);

	if (header.type != 2 && header.type != 3 && header.type != 10)
	{
		// ����Ή�����̂̓t���J���[�A�����A�t���J���[RLE���k�̂�
		//TODO: �Ȃ�����RLE���k��11�͑Ή����Ȃ��̂��H
		return false;
	}

	step += sizeof(unsigned char); // TODO:?
	step += 2 * 2 * sizeof(signed short); // x coordinate and y coordinate

	memcpy(&header.width, data + step, sizeof(signed short));
	memcpy(&header.height, data + step + sizeof(signed short), sizeof(signed short));
	memcpy(&header.pixelDepth, data + step + sizeof(signed short) * 2, sizeof(unsigned char));

	step += 2 * sizeof(signed short);
	step += sizeof(unsigned char);

	unsigned char garbage;
	memcpy(&garbage, data + step, sizeof(unsigned char));

	header.flipped = 0;
	if (garbage & 0x20)
	{
		header.flipped = 1;
	}
	// TODO:header����������Ȃ��ƁB�f�[�^�������ォ�炩���ق����Ȃ�����

	//
	// �f�[�^�����[�h
	//
	size_t headerSize = 6 * sizeof(unsigned char) + 6 * sizeof(signed short);
	int mode = header.pixelDepth / 8;

	if (header.type == 10)
	{
		// RLE���k�`���̂Ƃ�
		unsigned char runLength = 0;
		unsigned char color[4] = { 0 };
		step = headerSize;

		bool runLengthEncoded = false;
		unsigned int index = 0;

		unsigned int total = header.width * header.height;
		_rawDataLen = sizeof(unsigned char) * total * mode;
		_rawData = static_cast<unsigned char*>(malloc(_rawDataLen));
		for (unsigned int i = 0; i < total; ++i)
		{
			bool readPixel = false;

			// if run length is default, read in the run length token
			if (runLength == 0)
			{
				if (step + sizeof(unsigned char) > dataLen)
				{
					break;
				}

				memcpy(&runLength, data + step, sizeof(unsigned char));
				step += sizeof(unsigned char);

				// see if it's a RLE encoded sequence
				runLengthEncoded = ((runLength & 0x80) > 0);
				if (runLengthEncoded)
				{
					runLength -= 0x80;
				}

				readPixel = true;
			}
			// otherwise, we have a run length pending, run it
			else
			{
				runLength--;

				readPixel = !runLengthEncoded;
			}

			// do we need to skip reading this pixel?
			if (readPixel)
			{
				if (step + sizeof(unsigned char) * mode > dataLen)
				{
					break;
				}

				memcpy(color, data + step, sizeof(unsigned char) * mode);
				step += sizeof(unsigned char) * mode;

				// mode=3 or 4 implies that the image is RGB(A). However TGA
				// stores it as BGR(A) so we'll have to swap R and B.
				if (mode >= 3)
				{
					unsigned char tmpColor = color[0];
					color[0] = color[2];
					color[2] = tmpColor;
				}
			}

			// add the pixel to our image
			memcpy(&_rawData[index], color, mode);
			index += mode;
		}
	}
	else
	{
		step = headerSize;

		// RLE���k����Ă��Ȃ��Ƃ�
		_rawDataLen = sizeof(unsigned char) * header.width * header.height * mode;
		if (dataLen < headerSize + _rawDataLen)
		{
			return false;
		}

		_rawData = static_cast<unsigned char*>(malloc(_rawDataLen));
		memcpy(_rawData, data + step, _rawDataLen);

		// mode=3(or4)��RGA(A)�����ATGA��BGR(A)�`���Ȃ̂�R��B�����ւ��Ă���
		if (mode >= 3)
		{
			for (size_t i = 0; i < _rawDataLen; i += mode)
			{
				unsigned char b = _rawData[i];
				_rawData[i] = _rawData[i + 2];
				_rawData[i + 2] = b;
			}
		}
	}

	// TODO:initWithTGAData�̕����������ĂȂ�
	if (header.type == 2 || header.type == 10)
	{
		if (header.pixelDepth == 16)
		{
			_pixelFormat = TextureUtility::PixelFormat::RGB5A1;
		}
		else if (header.pixelDepth == 24)
		{
			_pixelFormat = TextureUtility::PixelFormat::RGB888;
		}
		else if (header.pixelDepth == 32)
		{
			_pixelFormat = TextureUtility::PixelFormat::RGBA8888;
		}
		else
		{
			Logger::log("Image WARNING: unsupport true color tga data pixel format.");
			return false;
		}
	}
	else if (header.type == 3)
	{
		if (header.pixelDepth == 8)
		{
			_pixelFormat = TextureUtility::PixelFormat::I8;
		}
		else
		{
			Logger::log("Image WARNING: unsupport gray tga data pixel format.");
			return false;
		}
	}

	_width = header.width;
	_height = header.height;
	_hasPremultipliedAlpha = false;
	return true;
}

void Image::premultiplyAlpha()
{
#if defined(MGRRENDERER_USE_OPENGL)
	Logger::logAssert(_pixelFormat == TextureUtility::PixelFormat::RGBA8888, "png�ł̓A���t�@�̎��O��Z��RGBA8888�ɂ����Ή������ĂȂ�");
#endif

	unsigned int* fourBytes = (unsigned int*)_rawData;
	for (int i = 0; i < _width * _height; i++)
	{
		unsigned char* p = _rawData + i * 4;
		// TODO:���ꂭ�炢�̂��Ƃ�GPU�ł���Ă������񂶂�Ȃ����Ȃ��B�܂��A�킴�킴�]������̂��Ȃ񂾂��ǁB
		fourBytes[i] = premultiplyAlpha(p[0], p[1], p[2], p[3]);
	}

	_hasPremultipliedAlpha = true;
}

} // namespace mgrrenderer
