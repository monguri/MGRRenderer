#pragma once
#include "Node.h"
#include "renderer/BasicDataTypes.h"
#include "renderer/CustomRenderCommand.h"
#include <vector>
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
	D3DProgram _d3dProgramForGBuffer;
	D3DProgram _d3dProgramForShadowMap;
	D3DProgram _d3dProgramForPointLightShadowMap;
	D3DProgram _d3dProgram;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgramForGBuffer;
	GLProgram _glProgram;
	GLProgram _glProgramForShadowMap;
#endif

	std::vector<Vec3> _vertexArray;
	std::vector<Vec3> _normalArray;

	CustomRenderCommand _renderGBufferCommand;
	CustomRenderCommand _renderShadowMapCommand[(size_t)CubeMapFace::NUM_CUBEMAP_FACE]; //TODO:openglでポイントライトの1パス未対応なので
	CustomRenderCommand _renderCommand;

	void renderGBuffer() override;
	void renderShadowMap(CubeMapFace face) override;
	void renderForward() override;
};

} // namespace mgrrenderer
