#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
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
	OpenGLProgramData _glData;
	CustomRenderCommand _renderCommand;
	std::vector<Vec3> _vertexArray;

	~Line3D();
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
