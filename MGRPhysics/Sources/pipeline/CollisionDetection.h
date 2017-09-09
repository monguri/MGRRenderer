#pragma once

namespace mgrphysics
{
struct State;
struct Collidable;
struct Pair;

// �Փˌ��o
// @param�@states ���̂̏�Ԃ̔z��
// @param�@collidables ���̂̌`��̔z��
// @param�@numRigidBodies ���̂̐�
// @param�@pairs �y�A�z��
// @param�@numPairs �y�A��
void detectCollision(
	const State* states,
	const Collidable* collidables,
	unsigned int numRigidBodies,
	const Pair* pairs,
	unsigned int numPairs
);
} // namespace mgrphysics
