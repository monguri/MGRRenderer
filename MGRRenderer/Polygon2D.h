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
	Polygon2D(const std::vector<Vec2>& vertexArray);
	~Polygon2D();

private:
	OpenGLProgramData _glData;
	std::vector<Vec2> _vertexArray;


	void render() override;
};

} // namespace mgrrenderer
