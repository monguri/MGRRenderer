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
	bool initWithModel(const Vec3& position, float scale, const std::string& filePath);
	void setTexture(const std::string& filePath);

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
	C3bLoader::MeshData* _meshData;
	size_t _perVertexByteSize;


	~Sprite3D();
	void render() override;
};

} // namespace mgrrenderer
