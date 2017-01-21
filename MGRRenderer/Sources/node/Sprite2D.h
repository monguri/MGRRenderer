#pragma once
#include "Node.h"
#include "renderer/BasicDataTypes.h"
#include "renderer/CustomRenderCommand.h"
#include "renderer/Director.h"
#include <string>
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "renderer/D3DProgram.h"
#elif defined(MGRRENDERER_USE_OPENGL)
#include "renderer/GLProgram.h"
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

	// TODO:�{����enum�𕪂���̂łȂ��}�e���A�����m�[�h����؂藣���ă}�e���A���������ŗ^����悤�ɂ�����
	enum class RenderBufferType : int
	{
		NONE = -1,
		DEPTH_TEXTURE,
		DEPTH_TEXTURE_ORTHOGONAL,
		DEPTH_CUBEMAP_TEXTURE,
		GBUFFER_COLOR_SPECULAR_INTENSITY,
		GBUFFER_NORMAL,
		GBUFFER_SPECULAR_POWER,
	};

	Sprite2D();
	virtual ~Sprite2D();
	bool init(const std::string& filePath);
#if defined(MGRRENDERER_USE_DIRECT3D)
	bool initWithTexture(D3DTexture* texture);
	// TODO:�{���̓��\�b�h�𕪂���̂łȂ��}�e���A�����m�[�h����؂藣���ă}�e���A���������ŗ^����悤�ɂ�����
	bool initWithRenderBuffer(D3DTexture* texture, RenderBufferType renderBufferType);
	bool initWithDepthStencilTexture(D3DTexture* texture, RenderBufferType renderBufferType, float nearClip, float farClip, const Mat4& projectionMatrix, CubeMapFace face = CubeMapFace::NONE);
#elif defined(MGRRENDERER_USE_OPENGL)
	bool initWithRenderBuffer(GLTexture* texture, RenderBufferType renderBufferType);
	bool initWithDepthStencilTexture(GLTexture* texture, RenderBufferType renderBufferType, float nearClip, float farClip, const Mat4& projectionMatrix, CubeMapFace face = CubeMapFace::NONE);
#endif

protected:
	RenderBufferType _renderBufferType;
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgramForForwardRendering;
	D3DTexture* _texture;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
	GLTexture* _texture;
#endif
	CustomRenderCommand _renderForwardCommand;
	Quadrangle2D _quadrangle;
#if defined(MGRRENDERER_USE_DIRECT3D)
	bool initCommon(const std::string& path, const std::string& vertexShaderFunctionName, const std::string& geometryShaderFunctionName, const std::string& pixelShaderFunctionName, const Size& contentSize);
#elif defined(MGRRENDERER_USE_OPENGL)
	bool initCommon(const std::string& geometryShaderFunctionPath, const std::string& pixelShaderFunctionPath, const Size& contentSize);
#endif

private:
	// �f�v�X�e�N�X�`���̓��j�A�f�v�X�ɖ߂��̂�3D�J�����̃v���W�F�N�V�����s�񂪕K�v�Ȃ̂ŁASprite2D��2D�J�����p�̃v���W�F�N�V�����s��̕ϐ��Ƌ�ʂ��Ă���
	static const std::string CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX;
	static const std::string CONSTANT_BUFFER_DEPTH_TEXTURE_PARAMETER;
	bool _isOwnTexture; // ���O�Ő��������e�N�X�`���ł���΂��̃N���X���ŉ������
	bool _isDepthTexture; // �f�v�X�e�N�X�`���������ꍇ
	float _nearClip;
	float _farClip;
	Mat4 _projectionMatrix;
	CubeMapFace _cubeMapFace;

#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void renderGBuffer() override;
#endif
	void renderForward() override;
};

} // namespace mgrrenderer
