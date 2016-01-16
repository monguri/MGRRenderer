#pragma once
#include "Node.h"
#include "BasicDataTypes.h"

namespace mgrrenderer
{

enum class LightType : int
{
	NONE = -1,

	AMBIENT,

	NUM_LIGHT_TYPE,
};

class Light :
	public Node
{
public:
	Light();
	~Light();

	virtual LightType getLightType() const = 0;
};

class AmbientLight :
	public Light
{
public:
	AmbientLight(const Color3B& color);

	LightType getLightType() const override { return LightType::AMBIENT; };
};

} // namespace mgrrenderer
