#pragma once

namespace mgrphysics
{
struct State;
struct Collidable;
struct Pair;

// 衝突検出
// @param　states 剛体の状態の配列
// @param　collidables 剛体の形状の配列
// @param　numRigidBodies 剛体の数
// @param　pairs ペア配列
// @param　numPairs ペア数
void detectCollision(
	const State* states,
	const Collidable* collidables,
	unsigned int numRigidBodies,
	const Pair* pairs,
	unsigned int numPairs
);
} // namespace mgrphysics
