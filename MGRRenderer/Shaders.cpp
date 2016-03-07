#include "Shaders.h"

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

} // namespace shader

} // namespace mgrrenderer
