#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include <vector>

namespace mgrrenderer
{

class Polygon3D
	: public Node
{
public:
	bool initWithVertexArray(const std::vector<Vec3>& vertexArray);

private:
	OpenGLProgramData _glData;
	std::vector<Vec3> _vertexArray;

	~Polygon3D();
	void render() override;
};

} // namespace mgrrenderer
