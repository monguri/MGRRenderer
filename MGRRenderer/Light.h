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
	struct ShadowMapData
	{
		Mat4 viewMatrix;
		GLuint frameBufferId;
		GLuint textureId;

		ShadowMapData() : frameBufferId(0), textureId(0) {};
	};

	DirectionalLight(const Vec3& direction, const Color3B& color);

	LightType getLightType() const override { return LightType::DIRECTION; };
	const Vec3& getDirection() const { return _direction; }
	const ShadowMapData& getShadowMapData() const { return _shadowMapData; }
	void prepareShadowMap(const Vec3& targetPosition, float cameraDistanceFromTarget, const Size& size);
	bool hasShadowMap() const { return _hasShadowMap; }

private:
	bool _hasShadowMap;
	Vec3 _direction;
	ShadowMapData _shadowMapData;
};

class PointLight :
	public Light
{
public:
	PointLight(const Vec3& position, const Color3B& color, float range);

	LightType getLightType() const override { return LightType::POINT; };
	float getRange() const { return _range; }

private:
	// 減衰率計算に用いる、光の届く範囲　正しい物理計算だと無限遠まで届くが、そうでないモデルをcocosが使ってるのでそれを採用
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
