#include "Shaders.h"
#if defined(MGRRENDERER_USE_OPENGL)

namespace mgrrenderer
{

namespace shader
{

#define STRINGIFY(A) #A

// VERTEX_SHADER_POSITION_MULTIPLY_COLOR
#include "VertexShaderPositionMultiplyColor.glsl"
// FRAGMENT_SHADER_POSITION_MULTIPLY_COLOR
#include "FragmentShaderPositionMultiplyColor.glsl"

// VERTEX_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR
#include "VertexShaderPositionTextureMultiplyColor.glsl"
// FRAGMENT_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR
#include "FragmentShaderPositionTextureMultiplyColor.glsl"

// VERTEX_SHADER_POSITION_NORMAL_MULTIPLY_COLOR
//#include "VertexShaderPositionNormalMultiplyColor.glsl"

// VERTEX_SHADER_POSITION_NORMAL_TEXTURE_MULTIPLY_COLOR
#include "VertexShaderPositionNormalTextureMultiplyColor.glsl"
// FRAGMENT_SHADER_POSITION_NORMAL_MULTIPLY_COLOR_GBUFFER
// FRAGMENT_SHADER_POSITION_NORMAL_TEXTURE_MULTIPLY_COLOR_GBUFFER
// FRAGMENT_SHADER_DEPTH_TEXTURE;
// FRAGMENT_SHADER_GBUFFER_COLOR_SPECULAR_INTENSITY;
// FRAGMENT_SHADER_GBUFFER_NORMAL;
// FRAGMENT_SHADER_GBUFFER_SPECULAR_POWER;
#include "FragmentShaderGBuffer.glsl"
} // namespace shader

} // namespace mgrrenderer
#endif
