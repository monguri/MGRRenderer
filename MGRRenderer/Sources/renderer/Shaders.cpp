#include "Shaders.h"
#if defined(MGRRENDERER_USE_OPENGL)

namespace mgrrenderer
{

namespace shader
{

#define STRINGIFY(A) #A

// VERTEX_SHADER_POSITION
#include "../Resources/shader/VertexShaderPosition.glsl"
// FRAGMENT_SHADER_POSITION_MULTIPLY_COLOR
#include "../Resources/shader/FragmentShaderPositionMultiplyColor.glsl"

// VERTEX_SHADER_POSITION_TEXTURE
#include "../Resources/shader/VertexShaderPositionTexture.glsl"
// FRAGMENT_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR
#include "../Resources/shader/FragmentShaderPositionTextureMultiplyColor.glsl"

// VERTEX_SHADER_POSITION_NORMAL
//#include "VertexShaderPositionNormal.glsl"

// FRAGMENT_SHADER_POSITION_NORMAL_MULTIPLY_COLOR_GBUFFER
// FRAGMENT_SHADER_POSITION_NORMAL_TEXTURE_MULTIPLY_COLOR_GBUFFER
// FRAGMENT_SHADER_DEPTH_TEXTURE;
// FRAGMENT_SHADER_GBUFFER_COLOR_SPECULAR_INTENSITY;
// FRAGMENT_SHADER_GBUFFER_NORMAL;
// FRAGMENT_SHADER_GBUFFER_SPECULAR_POWER;
#include "../Resources/shader/FragmentShaderGBuffer.glsl"

// VERTEX_SHADER_DEFERRED_LIGHTING;
#include "../Resources/shader/VertexShaderDeferredLighting.glsl"
// FRAGMENT_SHADER_DEFERRED_LIGHTING;
#include "../Resources/shader/FragmentShaderDeferredLighting.glsl"
} // namespace shader

} // namespace mgrrenderer
#endif
