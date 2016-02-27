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
		// TODO:–³ŒÀ•úo‚ª‚Å‚«‚Ä‚È‚¢B
		int numParticle;
		Vec3 gravity;
		float lifeTime;
		float initVelocity;
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
	std::vector<Vec3> _initVelocityArray;
	float _elapsedTime;

	GLint _uniformGravity;
	GLint _uniformLifeTime;
	GLint _attributeInitVelocity;
	GLint _uniformPointSize;
	GLint _uniformElapsedTime;

	~Particle3D();

	// TODO:‚Æ‚è‚ ‚¦‚¸‚Ü‚¾‚±‚ê‚Ì‰e¶¬‚Í‚µ‚È‚¢
	void update(float dt) override;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
