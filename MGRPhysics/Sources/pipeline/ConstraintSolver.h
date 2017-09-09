#pragma once

namespace mgrphysics
{
struct State;
struct Collidable;
struct Pair;
struct BallJoint;

// �S���\���o�[
// @param�@states ���̂̏�Ԃ̔z��
// @param�@collidables ���̂̌`��̔z��
// @param�@numRigidBodies ���̂̐�
// @param�@pairs �y�A�z��
// @param�@numpairs �y�A��
// @param�@joints �W���C���g�z��
// @param�@numJoints �W���C���g�A��
// @param�@iteration �v�Z�̔�����
// @param�@bias �ʒu�␳�̃o�C�A�X
// @param�@slop �ђʋ��e�덷
// @param�@timeStep �^�C���X�e�b�v
void solveConstraint(
	const State* states,
	const Collidable* collidables,
	unsigned int numRigidBodies,
	const Pair* pairs,
	unsigned int numPairs,
	BallJoint* joints,
	unsigned int numJoints,
	unsigned int iteration,
	float bias,
	float slop,
	float timeStep
);
} // namespace mgrphysics
