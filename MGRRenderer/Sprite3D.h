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
	// TODO:とりあえずフラグで動作を切り替えている
	bool _isObj;
	bool _isC3b;

	OpenGLProgramData _glData;
	//TODO: Textureは今のところモデルファイルで指定できない。一枚のみに対応
	Texture* _texture;
	
	// TODO:現状objのみに使っている。I/FをObjLoaderとC3bLoaderで合わせよう
	std::vector<Position3DTextureCoordinates> _vertices;

	// TODO:現状c3t/c3bのみに使っている。I/FをObjLoaderとC3bLoaderで合わせよう
	std::vector<unsigned short> _indices;
	C3bLoader::MeshData* _meshData;
	size_t _perVertexByteSize;


	~Sprite3D();
	void render() override;
};

} // namespace mgrrenderer
