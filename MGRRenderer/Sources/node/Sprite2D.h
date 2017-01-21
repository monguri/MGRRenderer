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
	friend Director; // Gバッファのデバッグ描画のためDirectorには公開している

	// TODO:本当はenumを分けるのでなくマテリアルをノードから切り離してマテリアルを引数で与えるようにしたい
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
	// TODO:本当はメソッドを分けるのでなくマテリアルをノードから切り離してマテリアルを引数で与えるようにしたい
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
	// デプステクスチャはリニアデプスに戻すのに3Dカメラのプロジェクション行列が必要なので、Sprite2Dの2Dカメラ用のプロジェクション行列の変数と区別している
	static const std::string CONSTANT_BUFFER_DEPTH_TEXTURE_PROJECTION_MATRIX;
	static const std::string CONSTANT_BUFFER_DEPTH_TEXTURE_PARAMETER;
	bool _isOwnTexture; // 自前で生成したテクスチャであればこのクラス内で解放する
	bool _isDepthTexture; // デプステクスチャを扱う場合
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
