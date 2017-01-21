#pragma once
#include "Node.h"
#include "renderer/GLProgram.h"
#include "renderer/CustomRenderCommand.h"

namespace mgrrenderer
{

#if defined(MGRRENDERER_USE_OPENGL)
class GLTexture;
#endif

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
	GLTexture* _texture;
#endif
	CustomRenderCommand _renderForwardCommand;
	Parameter _parameter;
	std::vector<Vec3> _vertexArray;
	std::vector<Vec3> _initVelocityArray;
	std::vector<float> _elapsedTimeArray;
	int _elapsedTimeMs; // msで扱ってintにしている

	~Particle3D();

	// TODO:とりあえずまだこれの影生成はしない
	void update(float dt) override;
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void renderGBuffer() override;
#endif
	void renderForward() override;
};

} // namespace mgrrenderer
