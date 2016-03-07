#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "GLProgram.h"
#include "CustomRenderCommand.h"
#include <vector>

namespace mgrrenderer
{


struct Point3DData
{
	Vec3 point;
	GLfloat pointSize;
	Point3DData(GLfloat x, GLfloat y, GLfloat z, GLfloat pointSize) : point(Vec3(x, y, z)), pointSize(pointSize) {}
};

class Point3D :
	public Node
{
public:
	void initWithPointArray(const std::vector<Point3DData>& pointArray);

private:
	GLProgram _glProgram;
	CustomRenderCommand _renderCommand;
	GLint _attributePointSize;

	std::vector<Point3DData> _pointArray;

	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
