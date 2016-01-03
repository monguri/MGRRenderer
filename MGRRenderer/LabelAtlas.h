#pragma once
#include "Node.h"
#include "BasicDataTypes.h"

namespace mgrrenderer
{

class Texture;

class LabelAtlas : public Node
{
public:
	LabelAtlas();
	~LabelAtlas();
	bool init(const std::string& string, const Texture* texture, int itemWidth, int itemHeight, char mapStartChararcter);
	void setString(const std::string& string);

private:
	OpenGLProgramData _glData;
	char _mapStartCharacter;
	std::string _string;
	const Texture* _texture;
	std::vector<Quadrangle2D> _quadrangles;
	int _itemWidth;
	int _itemHeight;

	void render() override;
};

} // namespace mgrrenderer
