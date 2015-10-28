#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include <vector>

namespace mgrrenderer
{

struct Vec2;

class Polygon2D
	: public Node
{
public:
	bool initWithVertexArray(const std::vector<Vec2>& vertexArray);

private:
	OpenGLProgramData _glData;
	std::vector<Vec2> _vertexArray;

	~Polygon2D();
	void render() override;
};

} // namespace mgrrenderer
