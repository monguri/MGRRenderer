#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "GLProgram.h"
#include "CustomRenderCommand.h"
#include <vector>

namespace mgrrenderer
{

class Line2D :
	public Node
{
public:
	bool initWithVertexArray(const std::vector<Vec2>& vertexArray);

private:
#if defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
#endif
	CustomRenderCommand _renderCommand;
	std::vector<Vec2> _vertexArray;

	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
