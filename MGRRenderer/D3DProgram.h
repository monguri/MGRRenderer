#pragma once
#include "Config.h"

#if defined(MGRRENDERER_USE_DIRECT3D)
#include <string>
#include <vector>
#include <d3d11.h>

namespace mgrrenderer
{

class D3DProgram final
{
public:
	// TODO:�����ɒu���̂����܂肢���Ƃ͎v��Ȃ����Ƃ肠����
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

	//void initWithShaderString(const std::string& shaderStr, bool depthTestEnable); // STRINGFY�ŕ����񈵂������Ⴄ��GPU�f�o�b�O�ł��Ȃ��Ȃ�̂ŃV�F�[�_�̓t�@�C���ň���
	// �V�F�[�_�֐������󕶎���ɂ����ꍇ�͂��̃V�F�[�_�͂Ȃ��ň����BsetShadersToDirect3DContext�ł�SetShader�ɂ�nullptr���Z�b�g����
	void initWithShaderFile(const std::string& path, bool depthTestEnable, const std::string& vertexShaderFunctionName, const std::string& geometryShaderFunctionName, const std::string& pixelShaderFunctionName);
	ID3D11VertexShader* getVertexShader() const { return _vertexShader; }
	ID3DBlob* getVertexShaderBlob() const { return _vertexShaderBlob; };
	ID3D11GeometryShader* getGeometryShader() const { return _geometryShader; }
	ID3D11PixelShader* getPixelShader() const { return _pixelShader; }
	ID3D11BlendState* getBlendState() const { return _blendState; }
	ID3D11RasterizerState* getRasterizeState() const { return _rasterizeState; }
	ID3D11DepthStencilState* getDepthStancilState() const { return _depthStencilState; }
	const std::vector<ID3D11Buffer*>& getVertexBuffers() const { return _vertexBuffers; }
	void addVertexBuffer(ID3D11Buffer* vertexBuffer) { _vertexBuffers.push_back(vertexBuffer); }
	ID3D11Buffer* getIndexBuffer() const { return _indexBuffer; }
	void setIndexBuffer(ID3D11Buffer* indexBuffer) { _indexBuffer = indexBuffer; }
	ID3D11InputLayout* getInputLayout() const { return _inputLayout; }
	void setInputLayout(ID3D11InputLayout* inputLayout) { _inputLayout = inputLayout; }
	const std::vector<ID3D11Buffer*>& getConstantBuffers() const { return _constantBuffers; }
	void addConstantBuffer(ID3D11Buffer* constantBuffer) { _constantBuffers.push_back(constantBuffer); }
	static DXGI_FORMAT getDxgiFormat(const std::string& semantic);

	void setShadersToDirect3DContext(ID3D11DeviceContext* context);
	void setConstantBuffersToDirect3DContext(ID3D11DeviceContext* context);

private:
	ID3D11VertexShader* _vertexShader;
	//TODO:���̓��C�A�E�g�͂܂����̒��ɉB�؂����ĂȂ��̂ŁA������O����A�N�Z�X������K�v������
	ID3DBlob* _vertexShaderBlob;
	ID3D11GeometryShader* _geometryShader;
	ID3D11PixelShader* _pixelShader;
	ID3D11BlendState* _blendState;
	ID3D11RasterizerState* _rasterizeState;
	ID3D11DepthStencilState* _depthStencilState;
	std::vector<ID3D11Buffer*> _vertexBuffers;
	ID3D11Buffer* _indexBuffer;
	ID3D11InputLayout* _inputLayout;
	std::vector<ID3D11Buffer*> _constantBuffers;
};

} // namespace mgrrenderer

#endif
