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
// コメントアウトしているものは#version読み込みのためにファイル読み込みする形に
//extern const char* VERTEX_SHADER_POSITION_NORMAL_MULTIPLY_COLOR;
//extern const char* VERTEX_SHADER_POSITION_NORMAL_TEXTURE_MULTIPLY_COLOR;
//extern const char* FRAGMENT_SHADER_POSITION_NORMAL_MULTIPLY_COLOR_GBUFFER;
//extern const char* FRAGMENT_SHADER_POSITION_NORMAL_TEXTURE_MULTIPLY_COLOR_GBUFFER;
extern const char* FRAGMENT_SHADER_DEPTH_TEXTURE;
extern const char* FRAGMENT_SHADER_GBUFFER_COLOR_SPECULAR_INTENSITY;
extern const char* FRAGMENT_SHADER_GBUFFER_NORMAL;
extern const char* FRAGMENT_SHADER_GBUFFER_SPECULAR_POWER;

} // namespace shader

} // namespace mgrrenderer
#endif
