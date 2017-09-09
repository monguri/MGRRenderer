#pragma once

// TODO:�C���N���[�h�p�X��MGRRenderer/sources��ǉ����Ă���B���ƂŊO��
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
using namespace mgrrenderer;

namespace mgrphysics
{
// �Փ˓_
struct Constraint
{
	// �S����
	Vec3 axis;
	// �S�����̕���
	float jacDiagInv;
	// �����S����
	float rhs;
	// �S���͂̉���
	float lowerLimit;
	// �S���͂̏��
	float upperLimit;
	// �~�ς����S����
	float accumImpulse;
};
} // namespace mgrphysics
