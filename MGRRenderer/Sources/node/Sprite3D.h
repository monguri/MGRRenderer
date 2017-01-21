#pragma once
#include <string>
#include <vector>
#include <array>
#include "Node.h"
#include "Light.h"
#include "renderer/CustomRenderCommand.h"
#include "loader/C3bLoader.h"
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

class Sprite3D :
	public Node
{
public:
	Sprite3D();
	bool initWithModel(const std::string& filePath);
	void setTexture(const std::string& filePath);
	void startAnimation(const std::string& animationName, bool loop = false);
	void stopAnimation();

private:
#if defined(MGRRENDERER_USE_DIRECT3D)
	static const int MAX_SKINNING_JOINT = 60; // シェーダと最大値定数を一致させること
#endif
	// TODO:とりあえずフラグで動作を切り替えている
	bool _isObj;
	bool _isC3b;

#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgram;
	D3DProgram _d3dProgramForShadowMap;
	D3DProgram _d3dProgramForPointLightShadowMap;
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	D3DProgram _d3dProgramForGBuffer;
#endif // defined(MGRRENDERER_DEFERRED_RENDERING)
	D3DTexture* _texture;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgramForGBuffer;
	GLProgram _glProgram;
	GLProgram _glProgramForShadowMap;
	GLTexture* _texture;
#endif
	CustomRenderCommand _renderGBufferCommand;
	CustomRenderCommand _renderDirectionalLightShadowMapCommand;
	std::array<std::array<CustomRenderCommand, (size_t)CubeMapFace::NUM_CUBEMAP_FACE>, PointLight::MAX_NUM> _renderPointLightShadowMapCommandList;
	std::array<CustomRenderCommand, SpotLight::MAX_NUM> _renderSpotLightShadowMapCommandList;
	CustomRenderCommand _renderForwardCommand;

	//TODO: Textureは今のところモデルファイルで指定できない。一枚のみに対応
	Color3F _ambient;
	Color3F _diffuse;
	Color3F _specular;
	float _shininess;
	//Color3F _emissive;
	//float _opacity;
	
	// TODO:現状objのみに使っている。I/FをObjLoaderとC3bLoaderで合わせよう
	std::vector<Position3DNormalTextureCoordinates> _vertices;

	std::vector<unsigned short> _indices;

	// TODO:現状c3t/c3bのみに使っている。I/FをObjLoaderとC3bLoaderで合わせよう
	C3bLoader::MeshDatas* _meshDatas;
	C3bLoader::NodeDatas* _nodeDatas;
	size_t _perVertexByteSize;
	C3bLoader::AnimationDatas* _animationDatas;
	C3bLoader::AnimationData* _currentAnimation;
	bool _loopAnimation;
	float _elapsedTime;
	std::vector<Mat4> _matrixPalette;

	~Sprite3D();
	void update(float dt) override;
	C3bLoader::NodeData* findJointByName(const std::string& jointName, const std::vector<C3bLoader::NodeData*> children);
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void renderGBuffer() override;
#endif
	void renderDirectionalLightShadowMap(const DirectionalLight* light) override;
	void renderPointLightShadowMap(size_t index, const PointLight* light, CubeMapFace face = CubeMapFace::X_POSITIVE) override;
	void renderSpotLightShadowMap(size_t index, const SpotLight* light) override;
	void renderForward() override;
};

} // namespace mgrrenderer
