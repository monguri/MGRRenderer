#include "Light.h"

namespace mgrrenderer
{

Light::Light() : _intensity(1.0f)
{
}

AmbientLight::AmbientLight(const Color3B& color)
{
	setColor(color);
}

DirectionalLight::DirectionalLight(const Vec3& direction, const Color3B& color) : _direction(direction)
{
	setColor(color);
}

PointLight::PointLight(const Vec3& position, const Color3B& color, float range) : _range(range)
{
	setPosition(position);
	setColor(color);
}

} // namespace mgrrenderer
