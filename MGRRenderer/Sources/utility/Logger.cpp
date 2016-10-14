#include "Logger.h"
//// TODO:va_list�Ȃǂ��g�����߂����A�����Ƃ܂��ȕ��@�͂Ȃ����ȁB�B
#include <ShlObj.h>
#include <stdio.h>
#include <assert.h>

namespace mgrrenderer
{

namespace Logger
{
	static void log(const char* format, va_list args)
	{
		char buf[MAX_LOG_LENGTH];
		vsnprintf_s(buf, MAX_LOG_LENGTH - 3, format, args); // TODO:�Ȃ�-3�Ȃ̂��Bcocos���̂܂܎����Ă��Ă邪
		strcat_s(buf, "\n");

		//TODO:�悭�킩���ĂȂ���cocos�̃R�s�y
		WCHAR wszBuf[MAX_LOG_LENGTH] = { 0 };
		MultiByteToWideChar(CP_UTF8, 0, buf, -1, wszBuf, sizeof(wszBuf));
		OutputDebugStringW(wszBuf);
		WideCharToMultiByte(CP_ACP, 0, wszBuf, -1, buf, sizeof(buf), nullptr, FALSE);
		printf("%s", buf);
		//SendLogToWindow(); // �K�v���킩��Ȃ������̂łƂ肠�����R�����g�A�E�g
		fflush(stdout);
	}

	void log(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		log(format, args);
		va_end(args);
	}

	void logAssert(bool equation, const char* format, ...)
	{
		if (!equation)
		{
			va_list args;
			va_start(args, format);
			log(format, args);
			va_end(args);
			assert(false);
		}
	}
} // namespace Logger

} // namespace mgrrenderer
