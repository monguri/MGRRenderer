#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
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
	OpenGLProgramData _glData;
	CustomRenderCommand _renderCommand;
	std::vector<Vec2> _vertexArray;

	~Line2D();
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
