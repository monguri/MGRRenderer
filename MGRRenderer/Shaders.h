#pragma once
#include "Config.h"
#if defined(MGRRENDERER_USE_OPENGL)
#include <gles/include/glew.h>

namespace mgrrenderer
{

namespace shader
{

extern const char* VERTEX_SHADER_POSITION_MULTIPLY_COLOR;
extern const char* FRAGMENT_SHADER_POSITION_MULTIPLY_COLOR;
extern const char* VERTEX_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR;
extern const char* FRAGMENT_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR;

} // namespace shader

} // namespace mgrrenderer
#endif
