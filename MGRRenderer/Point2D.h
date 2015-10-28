#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include <vector>

namespace mgrrenderer
{


struct Point2DData
{
	Vec2 point;
	GLfloat pointSize;
	Point2DData(GLfloat x, GLfloat y, GLfloat pointSize) : point(Vec2(x, y)), pointSize(pointSize) {}
};

class Point2D :
	public Node
{
public:
	void initWithPointArray(const std::vector<Point2DData>& pointArray);

private:
	OpenGLProgramData _glData;
	GLint _attributePointSize;

	std::vector<Point2DData> _pointArray;

	~Point2D();
	void render() override;
};

} // namespace mgrrenderer
