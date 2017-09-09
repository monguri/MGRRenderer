#pragma once

// TODO:�C���N���[�h�p�X��MGRRenderer/sources��ǉ����Ă���B���ƂŊO��
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
using namespace mgrrenderer;

namespace mgrphysics
{
struct State;
struct RigidBody;

// �S���\���o�[
// @param�@state ���̂̏��
// @param�@body ���̂̑���
// @param�@externalForce �^����t�H�[�X
// @param�@externalTorque �^����g���N
// @param�@timeStep �^�C���X�e�b�v
void applyExternalForce(
	State& state,
	RigidBody& body,
	const Vec3& externalForce,
	const Vec3& externalTorque,
	float timeStep
);

// �S���\���o�[
// @param�@states ���̂̏�Ԃ̔z��
// @param�@numRigidBodies ���̂̐�
// @param�@timeStep �^�C���X�e�b�v
void integrate(
	State* states,
	unsigned int numRigidBodies,
	float timeStep
);
} // namespace mgrphysics
