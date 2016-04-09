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
#endif
	CustomRenderCommand _renderCommand;
	Parameter _parameter;
	Texture* _texture;
	std::vector<Vec3> _vertexArray;
	std::vector<Vec3> _initVelocityArray;
	std::vector<float> _elapsedTimeArray;
	int _elapsedTimeMs; // ms�ň�����int�ɂ��Ă���

	~Particle3D();

	// TODO:�Ƃ肠�����܂�����̉e�����͂��Ȃ�
	void update(float dt) override;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
