#include "Light.h"

namespace mgrrenderer
{

Light::Light()
{
}


Light::~Light()
{
}

AmbientLight::AmbientLight(const Color3B& color)
{
	setColor(color);
}

} // namespace mgrrenderer
