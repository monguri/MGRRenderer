#pragma once
// TODO:ssize_t���g�����߂����A�����Ƃ܂��ȕ��@�͂Ȃ����ȁB�B
#include <ShlObj.h>
#ifndef __SSIZE_T
#define __SSIZE_T
typedef SSIZE_T ssize_t;
#endif // __SSIZE_T

namespace mgrrenderer
{

//����萔�����Ȃ��̂ŃN���X�����ĂȂ�
namespace FPSFontImage
{
	unsigned char PNG_DATA[];
	ssize_t getPngDataSize();
} // namespace FPSFontImage

} // namespace mgrrenderer
