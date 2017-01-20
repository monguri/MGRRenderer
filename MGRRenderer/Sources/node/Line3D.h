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

class Line3D :
	public Node
{
public:
	bool initWithVertexArray(const std::vector<Vec3>& vertexArray);

private:
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgram;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
#endif
	CustomRenderCommand _renderCommand;
	std::vector<Vec3> _vertexArray;

#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void renderGBuffer() override;
#endif
	void renderForward() override;
};

} // namespace mgrrenderer
