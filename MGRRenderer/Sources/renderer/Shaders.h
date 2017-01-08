#pragma once
#include "Config.h"
#if defined(MGRRENDERER_USE_OPENGL)
#define GLEW_STATIC
#include <gles/include/glew.h>

namespace mgrrenderer
{

namespace shader
{
	// glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);をしたため、#versionを書いてないとコンパイルエラーに
	// なるようになったので、STRINGIFYは使わなくなった
	// ファイルは一応残しておく
} // namespace shader

} // namespace mgrrenderer
#endif
