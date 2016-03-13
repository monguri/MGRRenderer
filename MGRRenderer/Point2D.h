#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "GLProgram.h"
#include "CustomRenderCommand.h"
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
	GLProgram _glProgram;
	CustomRenderCommand _renderCommand;

	std::vector<Point2DData> _pointArray;

	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
