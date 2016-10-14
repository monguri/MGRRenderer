#pragma once

namespace mgrrenderer
{

//現状staticメソッドしかないのでクラス化してない
namespace Logger
{
	static const int MAX_LOG_LENGTH = 16 * 1024; // cocosと同じにした
	void log(const char* format, ...);
	// TODO:ロガーに間借り
	void logAssert(bool equation, const char* format, ...);
} // namespace Logger

} // namespace mgrrenderer
