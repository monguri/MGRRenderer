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
	friend Director; // Gバッファのデバッグ描画のためDirectorには公開している

	Sprite2D();
	virtual ~Sprite2D();
	bool init(const std::string& filePath);
#if defined(MGRRENDERER_USE_DIRECT3D)
	bool initWithTexture(D3DTexture* texture);
	// TODO:本当はメソッドを分けるのでなくマテリアルをノードから切り離してマテリアルを引数で与えるようにしたい
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
	bool _isOwnTexture; // 自前で生成したテクスチャであればこのクラス内で解放する
	bool _isDepthTexture; // デプステクスチャを扱う場合

	void renderGBuffer() override;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
