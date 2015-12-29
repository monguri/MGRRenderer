#pragma once
#include <string>
#include <vector>
#include "Node.h"
#include "Texture.h"
#include "C3bLoader.h"

namespace mgrrenderer
{

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
	// TODO:�Ƃ肠�����t���O�œ����؂�ւ��Ă���
	bool _isObj;
	bool _isC3b;

	OpenGLProgramData _glData;
	//TODO: Texture�͍��̂Ƃ��냂�f���t�@�C���Ŏw��ł��Ȃ��B�ꖇ�݂̂ɑΉ�
	Texture* _texture;
	
	// TODO:����obj�݂̂Ɏg���Ă���BI/F��ObjLoader��C3bLoader�ō��킹�悤
	std::vector<Position3DTextureCoordinates> _vertices;

	// TODO:����c3t/c3b�݂̂Ɏg���Ă���BI/F��ObjLoader��C3bLoader�ō��킹�悤
	std::vector<unsigned short> _indices;
	C3bLoader::MeshDatas* _meshDatas;
	C3bLoader::NodeDatas* _nodeDatas;
	size_t _perVertexByteSize;
	C3bLoader::AnimationDatas* _animationDatas;
	C3bLoader::AnimationData* _currentAnimation;
	bool _loopAnimation;
	float _elapsedTime;
	std::vector<Mat4> _matrixPalette;
	//std::vector<Vec4> _matrixPalette;
	bool _isCpuMode;

	~Sprite3D();
	void update(float dt) override;
	void render() override;
	C3bLoader::NodeData* findJointByName(const std::string& jointName, const std::vector<C3bLoader::NodeData*> children);
};

} // namespace mgrrenderer
