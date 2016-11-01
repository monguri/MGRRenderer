#pragma once
#include <string>
#include <vector>
#include "Node.h"
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
	static const int MAX_SKINNING_JOINT = 60; // �V�F�[�_�ƍő�l�萔����v�����邱��
#endif
	// TODO:�Ƃ肠�����t���O�œ����؂�ւ��Ă���
	bool _isObj;
	bool _isC3b;

#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgram;
	D3DProgram _d3dProgramForShadowMap;
	D3DProgram _d3dProgramForGBuffer;
	D3DTexture* _texture;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgramForGBuffer;
	GLProgram _glProgram;
	GLProgram _glProgramForShadowMap;
	GLTexture* _texture;
#endif
	CustomRenderCommand _renderGBufferCommand;
	CustomRenderCommand _renderShadowMapCommand;
	CustomRenderCommand _renderCommand;
	//TODO: Texture�͍��̂Ƃ��냂�f���t�@�C���Ŏw��ł��Ȃ��B�ꖇ�݂̂ɑΉ�
	Color3F _ambient;
	Color3F _diffuse;
	Color3F _specular;
	float _shininess;
	//Color3F _emissive;
	//float _opacity;
	
	// TODO:����obj�݂̂Ɏg���Ă���BI/F��ObjLoader��C3bLoader�ō��킹�悤
	std::vector<Position3DNormalTextureCoordinates> _vertices;

	std::vector<unsigned short> _indices;

	// TODO:����c3t/c3b�݂̂Ɏg���Ă���BI/F��ObjLoader��C3bLoader�ō��킹�悤
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
	void renderGBuffer() override;
	void renderShadowMap() override;
	void renderForward() override;
};

} // namespace mgrrenderer