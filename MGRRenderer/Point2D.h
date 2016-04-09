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
	float pointSize;
	Point2DData(float x, float y, float pointSize) : point(Vec2(x, y)), pointSize(pointSize) {}
};

class Point2D :
	public Node
{
public:
	void initWithPointArray(const std::vector<Point2DData>& pointArray);

private:
#if defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
#endif
	CustomRenderCommand _renderCommand;

	std::vector<Point2DData> _pointArray;

	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
