#include "TextureUtility.h"
#include "Logger.h"

namespace mgrrenderer
{

namespace TextureUtility
{

PixelFormat convertDataToFormat(const unsigned char* data, ssize_t dataLen, PixelFormat fromFormat, PixelFormat toFormat, unsigned char** outData, ssize_t* outDataLen)
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

PixelFormat convertI8ToFormat(const unsigned char* data, ssize_t dataLen, PixelFormat toFormat, unsigned char** outData, ssize_t* outDataLen)
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

PixelFormat convertAI88ToFormat(const unsigned char* data, ssize_t dataLen, PixelFormat toFormat, unsigned char** outData, ssize_t* outDataLen)
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

PixelFormat convertRGB888ToFormat(const unsigned char* data, ssize_t dataLen, PixelFormat toFormat, unsigned char** outData, ssize_t* outDataLen)
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

PixelFormat convertRGBA8888ToFormat(const unsigned char* data, ssize_t dataLen, PixelFormat toFormat, unsigned char** outData, ssize_t* outDataLen)
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

void convertI8ToRGBA8888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0; i < dataLen; ++i)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i]; // G
		*outData++ = data[i]; // B
		*outData++ = 0xFF; // A
	}
}

void convertI8ToRGB888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0; i < dataLen; ++i)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i]; // G
		*outData++ = data[i]; // B
	}
}

void convertI8ToAI88(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	unsigned short* outData16 = (unsigned short*)outData;
	for (ssize_t i = 0; i < dataLen; ++i)
	{
		*outData16++ = 0xFF00 // A
			| data[i]; // I
	}
}

void convertI8ToRGBA4444(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
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

void convertAI88ToRGBA8888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 1; i < l; i += 2)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i]; // G
		*outData++ = data[i]; // B
		*outData++ = data[i + 1]; // A
	}
}

void convertAI88ToRGB888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 1; i < l; i += 2)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i]; // G
		*outData++ = data[i]; // B
	}
}

void convertAI88ToI8(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 1; i < l; i += 2)
	{
		*outData++ = data[i]; // R
	}
}

void convertAI88ToRGBA4444(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
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

void convertRGB888ToRGBA8888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 2; i < l; i += 3)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i + 1]; // G
		*outData++ = data[i + 2]; // B
		*outData++ = 0xFF; // A
	}
}

void convertRGB888ToI8(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 2; i < l; i += 3)
	{
		*outData++ = (data[i] * 299 + data[i + 1] * 587 + data[i + 2] * 114 + 500) / 1000; // I = (R * 299 + G * 587 + B * 114 + 500) / 1000 // TODO:どういう数式やろかこれ。。
	}
}

void convertRGB888ToAI88(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 2; i < l; i += 3)
	{
		*outData++ = (data[i] * 299 + data[i + 1] * 587 + data[i + 2] * 114 + 500) / 1000; // I = (R * 299 + G * 587 + B * 114 + 500) / 1000 // TODO:どういう数式やろかこれ。。
		*outData++ = 0xFF;
	}
}

void convertRGB888ToRGBA4444(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
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

void convertRGBA8888ToRGB888(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 3; i < l; i += 4)
	{
		*outData++ = data[i]; // R
		*outData++ = data[i + 1]; // G
		*outData++ = data[i + 2]; // B
	}
}

void convertRGBA8888ToI8(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 3; i < l; i += 4)
	{
		*outData++ = (data[i] * 299 + data[i + 1] * 587 + data[i + 2] * 114 + 500) / 1000; // I = (R * 299 + G * 587 + B * 114 + 500) / 1000 // TODO:どういう数式やろかこれ。。
	}
}

void convertRGBA8888ToAI88(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
{
	for (ssize_t i = 0, l = dataLen - 3; i < l; i += 4)
	{
		*outData++ = (data[i] * 299 + data[i + 1] * 587 + data[i + 2] * 114 + 500) / 1000; // I = (R * 299 + G * 587 + B * 114 + 500) / 1000 // TODO:どういう数式やろかこれ。。
		*outData++ = data[i + 3];
	}
}

void convertRGBA8888ToRGBA4444(const unsigned char* data, ssize_t dataLen, unsigned char* outData)
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

} // namespace TextureUtility

} // namespace mgrrenderer
