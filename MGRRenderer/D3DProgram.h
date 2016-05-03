#pragma once
#include "Config.h"

#if defined(MGRRENDERER_USE_DIRECT3D)
#include <d3dx11.h>
#include <string>

namespace mgrrenderer
{

class D3DProgram final
{
public:
	D3DProgram();
	~D3DProgram();

	void initWithShaderFile(const std::string& path);
	ID3D11VertexShader* getVertexShader() const { return _vertexShader; }
	ID3DBlob* getVertexShaderBlob() const { return _vertexShaderBlob; };
	ID3D11GeometryShader* getGeometryShader() const { return _geometryShader; }
	ID3D11PixelShader* getPixelShader() const { return _pixelShader; }
	ID3D11BlendState* getBlendState() const { return _blendState; }
	ID3D11RasterizerState* getRasterizeState() const { return _rasterizeState; }
	ID3D11DepthStencilState* getDepthStancilState() const { return _depthStencilState; }

private:
	ID3D11VertexShader* _vertexShader;
	//TODO:入力レイアウトはまだこの中に隠ぺいしてないので、これを外からアクセスさせる必要がある
	ID3DBlob* _vertexShaderBlob;
	ID3D11GeometryShader* _geometryShader;
	ID3D11PixelShader* _pixelShader;
	ID3D11BlendState* _blendState;
	ID3D11RasterizerState* _rasterizeState;
	ID3D11DepthStencilState* _depthStencilState;
};

} // namespace mgrrenderer

#endif
