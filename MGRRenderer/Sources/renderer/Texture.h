#pragma once

#include "Config.h"
#include "BasicDataTypes.h"
#include <map>

namespace mgrrenderer
{

namespace TextureUtility
{
enum class PixelFormat : int;
} // TextureUtility

class Image;

class Texture
{
public:
	Texture();
	virtual ~Texture() {}
	bool initWithImage(const Image& image);
	virtual bool initWithImage(const Image& image, TextureUtility::PixelFormat format) = 0;
	const Size& getContentSize() const { return _contentSize; }
	static TextureUtility::PixelFormat getDefaultPixelFormat();
	static void setDefaultPixelFormat(TextureUtility::PixelFormat format);

protected:
	Size _contentSize;
};

} // namespace mgrrenderer
