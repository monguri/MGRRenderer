#pragma once
#include <string>
#include <vector>
#include "Node.h"
#include "Texture.h"

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
	OpenGLProgramData _glData;
	//TODO: Textureは今のところモデルファイルで指定できない。一枚のみに対応
	Texture* _texture;
	std::vector<Position3DTextureCoordinates> _vertices;
	std::vector<unsigned short> _indices;

	~Sprite3D();
	void render() override;
};

} // namespace mgrrenderer
