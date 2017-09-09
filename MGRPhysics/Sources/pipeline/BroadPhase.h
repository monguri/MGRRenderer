#pragma once

namespace mgrphysics
{
struct State;
struct Collidable;
struct Pair;

// �u���[�h�t�F�C�Y
// ����C����I�ȌÂ������������ACPU�l�b�N�Ȃ̂�CPU�������Ȃ�ׂ��y������
// @param�@states ���̂̏�Ԃ̔z��
// @param�@collidables ���̂̌`��̔z��
// @param�@numRigidBodies ���̂̐�
// @param�@oldPairs �O�t���[���̃y�A�z��
// @param�@numOldPairs �O�t���[���̃y�A��
void broadPhase(
	const State* states,
	const Collidable* collidables,
	unsigned int numRigidBodies,
	Pair* oldPairs,
	unsigned int numOldPairs,
	Pair* newPairs,
	unsigned int numNewPairs,
	const unsigned int maxPairs,
	void* userData
);
} // namespace mgrphysics
