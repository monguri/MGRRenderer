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


struct Point3DData
{
	Vec3 point;
	float pointSize;
	Point3DData(float x, float y, float z, float pointSize) : point(Vec3(x, y, z)), pointSize(pointSize) {}
};

class Point3D :
	public Node
{
public:
	void initWithPointArray(const std::vector<Point3DData>& pointArray);

private:
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgram;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
#endif
	CustomRenderCommand _renderCommand;

	std::vector<Point3DData> _pointArray;

#if defined(MGRRENDERER_DEFFERED_RENDERING)
	void renderGBuffer() override;
#endif
	void renderForward() override;
};

} // namespace mgrrenderer
