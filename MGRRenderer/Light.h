#pragma once
#include "Node.h"
#include "BasicDataTypes.h"

namespace mgrrenderer
{

enum class LightType : int
{
	NONE = -1,

	AMBIENT,
	DIRECTION,
	POINT,

	NUM_LIGHT_TYPE,
};

class Light :
	public Node
{
public:
	// 直接Lightのインスタンスを作ることはできない
	virtual LightType getLightType() const = 0;
	float getIntensity() const { return _intensity; }
	void setIntensity(float intensity) { _intensity = intensity; }

protected:
	Light();

private:
	float _intensity;
};

class AmbientLight :
	public Light
{
public:
	AmbientLight(const Color3B& color);

	LightType getLightType() const override { return LightType::AMBIENT; };
};

class DirectionalLight :
	public Light
{
public:
	DirectionalLight(const Vec3& direction, const Color3B& color);

	LightType getLightType() const override { return LightType::DIRECTION; };
	const Vec3& getDirection() const { return _direction; }

private:
	Vec3 _direction;
};

class PointLight :
	public Light
{
public:
	PointLight(const Vec3& position, const Color3B& color, float range);

	LightType getLightType() const override { return LightType::POINT; };
	float getRange() const { return _range; }

private:
	float _range;
};

} // namespace mgrrenderer
