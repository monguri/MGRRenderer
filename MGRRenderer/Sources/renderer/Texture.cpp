#include "Texture.h"
#include "TextureUtility.h"

namespace mgrrenderer
{

static TextureUtility::PixelFormat _defaultPixelFormat = TextureUtility::PixelFormat::DEFAULT;

Texture::Texture() : _contentSize(Size(0, 0))
{
}

bool Texture::initWithImage(const Image& image)
{
	return initWithImage(image, _defaultPixelFormat);
}

TextureUtility::PixelFormat Texture::getDefaultPixelFormat()
{
	return _defaultPixelFormat;
}

void Texture::setDefaultPixelFormat(TextureUtility::PixelFormat format)
{
	_defaultPixelFormat = format;
}

} // namespace mgrrenderer
