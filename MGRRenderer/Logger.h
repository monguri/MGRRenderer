#pragma once

namespace mgrrenderer
{

//����static���\�b�h�����Ȃ��̂ŃN���X�����ĂȂ�
namespace Logger
{
	static const int MAX_LOG_LENGTH = 16 * 1024; // cocos�Ɠ����ɂ���
	void log(const char* format, ...);
	// TODO:���K�[�ɊԎ؂�
	void logAssert(bool equation, const char* format, ...);
} // namespace Logger

} // namespace mgrrenderer
