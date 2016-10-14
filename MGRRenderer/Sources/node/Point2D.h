#pragma once
#include "Node.h"
#include "renderer/BasicDataTypes.h"
#include "renderer/CustomRenderCommand.h"
#include <vector>
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "renderer/D3DProgram.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "renderer/GLProgram.h"
#endif

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
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgram;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
#endif
	CustomRenderCommand _renderCommand;

	std::vector<Point2DData> _pointArray;

	void renderGBuffer() override;
	void renderForward() override;
};

} // namespace mgrrenderer
