#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "CustomRenderCommand.h"
#include <vector>
#if defined(MGRRENDERER_USE_DIRECT3D)
#include <d3dx11.h>
#include "D3DProgram.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "GLProgram.h"
#endif

namespace mgrrenderer
{

struct Vec2;

class Polygon2D : public Node
{
public:
#if defined(MGRRENDERER_USE_DIRECT3D)
	Polygon2D();
	~Polygon2D();
#endif
	bool initWithVertexArray(const std::vector<Vec2>& vertexArray);

private:
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgram;
	ID3D11Buffer* _vertexBuffer;
	ID3D11Buffer* _indexBuffer;
	ID3D11InputLayout* _inputLayout;
	ID3D11Buffer* _constantBuffers[4];
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
#endif
	CustomRenderCommand _renderCommand;
	std::vector<Vec2> _vertexArray;

	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
