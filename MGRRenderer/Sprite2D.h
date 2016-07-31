#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "CustomRenderCommand.h"
#include "Director.h"
#include <string>
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DProgram.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "GLProgram.h"
#endif

namespace mgrrenderer
{

#if defined(MGRRENDERER_USE_DIRECT3D)
class D3DTexture;
#elif defined(MGRRENDERER_USE_OPENGL)
class GLTexture;
#endif

namespace TextureUtility
{
enum class PixelFormat : int;
} // TextureUtility

class Sprite2D :
	public Node
{
public:
	friend Director; // G�o�b�t�@�̃f�o�b�O�`��̂���Director�ɂ͌��J���Ă���

	Sprite2D();
	virtual ~Sprite2D();
	bool init(const std::string& filePath);
#if defined(MGRRENDERER_USE_DIRECT3D)
	bool initWithTexture(D3DTexture* texture);
	// TODO:�{���̓��\�b�h�𕪂���̂łȂ��}�e���A�����m�[�h����؂藣���ă}�e���A���������ŗ^����悤�ɂ�����
	bool initWithDepthTexture(D3DTexture* texture);
#elif defined(MGRRENDERER_USE_OPENGL)
	bool initWithTexture(GLTexture* texture);
#endif

protected:
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgram;
	D3DTexture* _texture;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
	GLTexture* _texture;
#endif
	CustomRenderCommand _renderCommand;
	Quadrangle2D _quadrangle;
#if defined(MGRRENDERER_USE_DIRECT3D)
	bool initCommon(const std::string& path, const std::string& vertexShaderFunctionName, const std::string& geometryShaderFunctionName, const std::string& pixelShaderFunctionName, const Size& contentSize);
#elif defined(MGRRENDERER_USE_OPENGL)
	bool initCommon(const std::string& path, const Size& contentSize);
#endif

private:
	static const std::string CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX;
	bool _isOwnTexture; // ���O�Ő��������e�N�X�`���ł���΂��̃N���X���ŉ������
	bool _isDepthTexture; // �f�v�X�e�N�X�`���������ꍇ

	void renderGBuffer() override;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
