#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "GLProgram.h"
#include "CustomRenderCommand.h"
#include <vector>
#if defined(MGRRENDERER_USE_DIRECT3D)
#include <d3dx11.h>
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
	ID3D11Buffer* _vertexBuffer;
	ID3D11Buffer* _indexBuffer;
	ID3D11VertexShader* _vertexShader;
	ID3D11InputLayout* _inputLayout;
	ID3D11GeometryShader* _geometryShader;
	ID3D11PixelShader* _pixelShader;
	ID3D11Buffer* _constantBuffers[4];
	ID3D11BlendState* _blendState;
	ID3D11RasterizerState* _rasterizeState;
	ID3D11DepthStencilState* _depthStencilState;
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
