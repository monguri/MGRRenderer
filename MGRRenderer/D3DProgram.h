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
	// TODO:ここに置くのがあまりいいとは思わないがとりあえず
	static const std::string SEMANTIC_POSITION;
	static const std::string SEMANTIC_COLOR;
	static const std::string SEMANTIC_TEXTURE_COORDINATE;
	static const std::string SEMANTIC_TEXTURE_COORDINATE_1;
	static const std::string SEMANTIC_TEXTURE_COORDINATE_2;
	static const std::string SEMANTIC_TEXTURE_COORDINATE_3;
	static const std::string SEMANTIC_NORMAL;
	static const std::string SEMANTIC_BLEND_WEIGHT;
	static const std::string SEMANTIC_BLEND_INDEX;

	D3DProgram();
	~D3DProgram();

	//void initWithShaderString(const std::string& shaderStr, bool depthTestEnable); // STRINGFYで文字列扱いしちゃうとGPUデバッグできなくなるのでシェーダはファイルで扱う
	void initWithShaderFile(const std::string& path, bool depthTestEnable);
	ID3D11VertexShader* getVertexShader() const { return _vertexShader; }
	ID3DBlob* getVertexShaderBlob() const { return _vertexShaderBlob; };
	ID3D11GeometryShader* getGeometryShader() const { return _geometryShader; }
	ID3D11PixelShader* getPixelShader() const { return _pixelShader; }
	ID3D11BlendState* getBlendState() const { return _blendState; }
	ID3D11RasterizerState* getRasterizeState() const { return _rasterizeState; }
	ID3D11DepthStencilState* getDepthStancilState() const { return _depthStencilState; }
	static DXGI_FORMAT getDxgiFormat(const std::string& semantic);

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
