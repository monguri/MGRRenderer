#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include <vector>

namespace mgrrenderer
{

struct Vec2;

class Line2D :
	public Node
{
public:
	bool initWithVertexArray(const std::vector<Vec2>& vertexArray);

private:
	~Line2D();
	OpenGLProgramData _glData;
	std::vector<Vec2> _vertexArray;

	void render() override;
};

} // namespace mgrrenderer
