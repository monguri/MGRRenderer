#pragma once
#include "Node.h"
#include "GLProgram.h"
#include "CustomRenderCommand.h"

namespace mgrrenderer
{

class Texture;

class Particle3D :
	public Node
{
public:
	struct Parameter {
		bool loopFlag;
		// ループモードの時は秒間あたりの放出数。そうでないときは全放出数。
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
#if defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgram;
#endif
	CustomRenderCommand _renderCommand;
	Parameter _parameter;
	Texture* _texture;
	std::vector<Vec3> _vertexArray;
	std::vector<Vec3> _initVelocityArray;
	std::vector<float> _elapsedTimeArray;
	int _elapsedTimeMs; // msで扱ってintにしている

	~Particle3D();

	// TODO:とりあえずまだこれの影生成はしない
	void update(float dt) override;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
