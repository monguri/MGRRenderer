#pragma once
// TODO:ssize_tを使うためだが、もっとましな方法はないかな。。
#include <ShlObj.h>
#ifndef __SSIZE_T
#define __SSIZE_T
typedef SSIZE_T ssize_t;
#endif // __SSIZE_T

namespace mgrrenderer
{

//現状定数しかないのでクラス化してない
namespace FPSFontImage
{
	unsigned char PNG_DATA[];
	ssize_t getPngDataSize();
} // namespace FPSFontImage

} // namespace mgrrenderer
