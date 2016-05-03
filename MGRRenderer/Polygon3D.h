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

class Polygon3D
	: public Node
{
public:
#if defined(MGRRENDERER_USE_DIRECT3D)
	Polygon3D();
	~Polygon3D();
#endif
	bool initWithVertexArray(const std::vector<Vec3>& vertexArray);

private:
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgram;
	ID3D11Buffer* _vertexBuffer;
	ID3D11Buffer* _indexBuffer;
	ID3D11InputLayout* _inputLayout;
	ID3D11Buffer* _constantBuffers[4];
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
	GLProgram _glProgramForShadowMap;
#endif

	std::vector<Vec3> _vertexArray;
	std::vector<Vec3> _normalArray;

	CustomRenderCommand _renderShadowMapCommand;
	CustomRenderCommand _renderCommand;

	void renderShadowMap() override;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
