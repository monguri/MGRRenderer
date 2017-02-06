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
		// ���[�v���[�h�̎��͕b�Ԃ�����̕��o���B�����łȂ��Ƃ��͑S���o���B
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
	int _elapsedTimeMs; // ms�ň�����int�ɂ��Ă���

	~Particle3D();

	// TODO:�Ƃ肠�����܂�����̉e�����͂��Ȃ�
	void update(float dt) override;
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void renderGBuffer() override;
#endif
	void renderForward() override;
};

} // namespace mgrrenderer
