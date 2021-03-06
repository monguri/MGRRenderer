#include "Logger.h"
//// TODO:va_listなどを使うためだが、もっとましな方法はないかな。。
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
		vsnprintf_s(buf, MAX_LOG_LENGTH - 3, format, args); // TODO:なぜ-3なのか。cocosそのまま持ってきてるが
		strcat_s(buf, "\n");

		//TODO:よくわかってないがcocosのコピペ
		WCHAR wszBuf[MAX_LOG_LENGTH] = { 0 };
		MultiByteToWideChar(CP_UTF8, 0, buf, -1, wszBuf, sizeof(wszBuf));
		OutputDebugStringW(wszBuf);
		WideCharToMultiByte(CP_ACP, 0, wszBuf, -1, buf, sizeof(buf), nullptr, FALSE);
		printf("%s", buf);
		//SendLogToWindow(); // 必要かわからなかったのでとりあえずコメントアウト
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
