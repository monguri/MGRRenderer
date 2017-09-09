#pragma once

// TODO:�C���N���[�h�p�X��MGRRenderer/sources��ǉ����Ă���B���ƂŊO��
#include "renderer/BasicDataTypes.h"
#include "Constraint.h"

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
using namespace mgrrenderer;

namespace mgrphysics
{
struct BallJoint
{
	// �S���̋����̒����l
	float bias;
	// ����A�̃C���f�b�N�X
	unsigned int rigidBodyA;
	// ����B�̃C���f�b�N�X
	unsigned int rigidBodyB;
	// ����A�̃��[�J�����W�ɂ�����ڑ��_
	Vec3 anchorA;
	// ����B�̃��[�J�����W�ɂ�����ڑ��_
	Vec3 anchorB;
	// �S��
	Constraint constraint;

	void reset()
	{
		bias = 0.1f;
		constraint.accumImpulse = 0.0f;
	}
};
} // namespace mgrphysics
