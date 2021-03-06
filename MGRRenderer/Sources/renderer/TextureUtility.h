#pragma once

namespace mgrrenderer
{

namespace TextureUtility
{
	enum class PixelFormat : int
	{
		NONE = -1,

		//! auto detect the type
		AUTO,
		////! 32-bit texture: BGRA8888
		//BGRA8888,
		//! 32-bit texture: RGBA8888
		RGBA8888,
		//! 24-bit texture: RGBA888
		RGB888,
		//! 16-bit texture without Alpha channel
		//RGB565,
		////! 8-bit textures used as masks
		//A8,
		//! 8-bit intensity texture
		I8,
		//! 16-bit textures used as masks
		AI88,
		////! 16-bit textures: RGBA4444
		RGBA4444,
		//! 16-bit textures: RGB5A1
		RGB5A1,
		////! 4-bit PVRTC-compressed texture: PVRTC4
		//PVRTC4,
		////! 4-bit PVRTC-compressed texture: PVRTC4 (has alpha channel)
		//PVRTC4A,
		////! 2-bit PVRTC-compressed texture: PVRTC2
		//PVRTC2,
		////! 2-bit PVRTC-compressed texture: PVRTC2 (has alpha channel)
		//PVRTC2A,
		////! ETC-compressed texture: ETC
		//ETC,
		////! S3TC-compressed texture: S3TC_Dxt1
		//S3TC_DXT1,
		////! S3TC-compressed texture: S3TC_Dxt3
		//S3TC_DXT3,
		////! S3TC-compressed texture: S3TC_Dxt5
		//S3TC_DXT5,
		////! ATITC-compressed texture: ATC_RGB
		//ATC_RGB,
		////! ATITC-compressed texture: ATC_EXPLICIT_ALPHA
		//ATC_EXPLICIT_ALPHA,
		////! ATITC-compresed texture: ATC_INTERPOLATED_ALPHA
		//ATC_INTERPOLATED_ALPHA,
		//! Default texture format: AUTO
		DEFAULT = AUTO,

		NUM_PIXEL_FORMATS,
	};

	PixelFormat convertDataToFormat(const unsigned char* data, size_t dataLen, PixelFormat fromFormat, PixelFormat toFormat, unsigned char** outData, size_t* outDataLen);
	PixelFormat convertI8ToFormat(const unsigned char* data, size_t dataLen, PixelFormat toFormat, unsigned char** outData, size_t* outDataLen);
	PixelFormat convertAI88ToFormat(const unsigned char* data, size_t dataLen, PixelFormat toFormat, unsigned char** outData, size_t* outDataLen);
	PixelFormat convertRGB888ToFormat(const unsigned char* data, size_t dataLen, PixelFormat toFormat, unsigned char** outData, size_t* outDataLen);
	PixelFormat convertRGBA8888ToFormat(const unsigned char* data, size_t dataLen, PixelFormat toFormat, unsigned char** outData, size_t* outDataLen);
	void convertI8ToRGBA8888(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertI8ToRGB888(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertI8ToAI88(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertI8ToRGBA4444(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertAI88ToRGBA8888(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertAI88ToRGB888(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertAI88ToI8(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertAI88ToRGBA4444(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertRGB888ToRGBA8888(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertRGB888ToI8(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertRGB888ToAI88(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertRGB888ToRGBA4444(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertRGBA8888ToRGB888(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertRGBA8888ToI8(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertRGBA8888ToAI88(const unsigned char* data, size_t dataLen, unsigned char* outData);
	void convertRGBA8888ToRGBA4444(const unsigned char* data, size_t dataLen, unsigned char* outData);
} // namespace TextureUtility

} // namespace mgrrenderer
