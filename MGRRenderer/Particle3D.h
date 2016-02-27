#pragma once
#include "Node.h"

namespace mgrrenderer
{

class Texture;

class Particle3D :
	public Node
{
public:
	struct Parameter {
		int numParticle;
		Vec3 gravity;
		float lifeTime;
		Vec3 initVelocity;
		std::string textureFilePath;
		float pointSize;
	};

	Particle3D();
	bool initWithParameter(const Parameter& parameter);

private:
	OpenGLProgramData _glData;
	Parameter _parameter;
	Texture* _texture;
	std::vector<Vec3> _vertexArray;
	float _elapsedTime;

	GLint _uniformGravity;
	GLint _uniformLifeTime;
	GLint _uniformInitVelocity;
	GLint _uniformPointSize;
	GLint _uniformElapsedTime;

	~Particle3D();

	// TODO:Ç∆ÇËÇ†Ç¶Ç∏Ç‹ÇæÇ±ÇÍÇÃâeê∂ê¨ÇÕÇµÇ»Ç¢
	void update(float dt) override;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
