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
	SPOT,

	NUM_LIGHT_TYPE,
};

class Light :
	public Node
{
public:
	// ����Light�̃C���X�^���X����邱�Ƃ͂ł��Ȃ�
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
	// �������v�Z�ɗp����A���̓͂��͈́@�����������v�Z���Ɩ������܂œ͂����A�����łȂ����f����cocos���g���Ă�̂ł�����̗p
	float _range;
};

class SpotLight :
	public Light
{
public:
	SpotLight(const Vec3& position, const Vec3& direction, const Color3B& color, float range, float innerAngle, float outerAngle);

	LightType getLightType() const override { return LightType::SPOT; };
	const Vec3& getDirection() const { return _direction; }
	float getRange() const { return _range; }
	float getInnerAngleCos() const { return _innerAngleCos; }
	float getOuterAngleCos() const { return _outerAngleCos; }

private:
	Vec3 _direction;
	float _range;
	float _innerAngleCos;
	float _outerAngleCos;
};

} // namespace mgrrenderer
