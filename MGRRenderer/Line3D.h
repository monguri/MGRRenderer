#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include <vector>

namespace mgrrenderer
{

class Line3D :
	public Node
{
public:
	bool initWithVertexArray(const std::vector<Vec3>& vertexArray);

private:
	~Line3D();
	OpenGLProgramData _glData;
	std::vector<Vec3> _vertexArray;

	void render() override;
};

} // namespace mgrrenderer
