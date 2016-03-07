#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "GLProgram.h"
#include "CustomRenderCommand.h"
#include <vector>

namespace mgrrenderer
{

class Line3D :
	public Node
{
public:
	bool initWithVertexArray(const std::vector<Vec3>& vertexArray);

private:
	GLProgram _glProgram;
	CustomRenderCommand _renderCommand;
	std::vector<Vec3> _vertexArray;

	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
