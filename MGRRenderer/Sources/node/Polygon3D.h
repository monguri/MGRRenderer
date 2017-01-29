#pragma once
#include "Node.h"
#include "Light.h"
#include "renderer/BasicDataTypes.h"
#include "renderer/CustomRenderCommand.h"
#include <vector>
#include <array>
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "renderer/D3DProgram.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "renderer/GLProgram.h"
#endif

namespace mgrrenderer
{

class Polygon3D
	: public Node
{
public:
	bool initWithVertexArray(const std::vector<Vec3>& vertexArray);

private:
#if defined(MGRRENDERER_USE_DIRECT3D)
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	D3DProgram _d3dProgramForGBuffer;
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)
	D3DProgram _d3dProgramForShadowMap;
	D3DProgram _d3dProgramForPointLightShadowMap;
	D3DProgram _d3dProgramForForwardRendering;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgramForGBuffer;
	GLProgram _glProgram;
	GLProgram _glProgramForShadowMap;
#endif

	std::vector<Vec3> _vertexArray;
	std::vector<Vec3> _normalArray;

#if defined(MGRRENDERER_DEFERRED_RENDERING)
	CustomRenderCommand _renderGBufferCommand;
#endif
	CustomRenderCommand _renderDirectionalLightShadowMapCommand;
	std::array<std::array<CustomRenderCommand, (size_t)CubeMapFace::NUM_CUBEMAP_FACE>, PointLight::MAX_NUM> _renderPointLightShadowMapCommandList;
	std::array<CustomRenderCommand, SpotLight::MAX_NUM> _renderSpotLightShadowMapCommandList;
	CustomRenderCommand _renderForwardCommand;

#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void renderGBuffer() override;
#endif
	void renderDirectionalLightShadowMap(const DirectionalLight* light) override;
	void renderPointLightShadowMap(size_t index, const PointLight* light, CubeMapFace face = CubeMapFace::X_POSITIVE) override;
	void renderSpotLightShadowMap(size_t index, const SpotLight* light) override;
	void renderForward(bool isTransparent) override;
};

} // namespace mgrrenderer
