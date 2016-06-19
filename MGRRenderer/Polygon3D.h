#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "CustomRenderCommand.h"
#include <vector>
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DProgram.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "GLProgram.h"
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
	enum class ConstantBufferIndex : int {
		MODEL_MATRIX = 0,
		VIEW_MATRIX,
		PROJECTION_MATRIX,
		MULTIPLY_COLOR,
		AMBIENT_LIGHT_PARAMETER,
		DIRECTIONAL_LIGHT_VIEW_MATRIX,
		DIRECTIONAL_LIGHT_PROJECTION_MATRIX,
		DIRECTIONAL_LIGHT_DEPTH_BIAS_MATRIX,
		DIRECTIONAL_LIGHT_PARAMETER,
		POINT_LIGHT_PARAMETER,
		SPOT_LIGHT_PARAMETER,
	};

	D3DProgram _d3dProgram;
	D3DProgram _d3dProgramForShadowMap;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
	GLProgram _glProgramForShadowMap;
#endif

	std::vector<Vec3> _vertexArray;
	std::vector<Vec3> _normalArray;

	CustomRenderCommand _renderShadowMapCommand;
	CustomRenderCommand _renderCommand;

	void renderShadowMap() override;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
