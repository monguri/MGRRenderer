#pragma once

namespace mgrphysics
{
struct State;
struct Collidable;
struct Pair;

// ブロードフェイズ
// 非常にC言語的な古い書き方だが、CPUネックなのでCPU処理をなるべく軽く書く
// @param　states 剛体の状態の配列
// @param　collidables 剛体の形状の配列
// @param　numRigidBodies 剛体の数
// @param　oldPairs 前フレームのペア配列
// @param　numOldPairs 前フレームのペア数
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
