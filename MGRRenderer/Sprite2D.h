#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "Texture.h"
#include <string>

namespace mgrrenderer
{

class Sprite2D :
	public Node
{
public:
	Sprite2D();
	~Sprite2D();
	bool init(const Vec2& position, const std::string& filePath);

private:
	Rect _rect;
	OpenGLProgramData _glData;
	Texture* _texture;
	Quadrangle2D _quadrangle;

	void render() override;
};

} // namespace mgrrenderer