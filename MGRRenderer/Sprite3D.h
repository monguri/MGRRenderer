#pragma once
#include <string>
#include <vector>
#include "Node.h"
#include "GLProgram.h"
#include "CustomRenderCommand.h"
#include "C3bLoader.h"

#if defined(MGRRENDERER_USE_OPENGL)
namespace mgrrenderer
{

class Texture;

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
	// TODO:とりあえずフラグで動作を切り替えている
	bool _isObj;
	bool _isC3b;

#if defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
	GLProgram _glProgramForShadowMap;
#endif
	CustomRenderCommand _renderShadowMapCommand;
	CustomRenderCommand _renderCommand;
	//TODO: Textureは今のところモデルファイルで指定できない。一枚のみに対応
	Texture* _texture;
	Color3F _ambient;
	Color3F _diffuse;
	Color3F _specular;
	float _shininess;
	//Color3F _emissive;
	//float _opacity;
	
	// TODO:現状objのみに使っている。I/FをObjLoaderとC3bLoaderで合わせよう
	std::vector<Position3DNormalTextureCoordinates> _vertices;

	// TODO:現状c3t/c3bのみに使っている。I/FをObjLoaderとC3bLoaderで合わせよう
	std::vector<unsigned short> _indices;
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
	void renderShadowMap() override;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
#endif
