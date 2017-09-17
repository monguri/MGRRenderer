#pragma once

namespace mgrphysics
{
struct State;
struct RigidBody;
struct Pair;
struct BallJoint;

// 拘束ソルバー
// @param　states 剛体の状態の配列
// @param　bodies 剛体の属性の配列
// @param　numRigidBodies 剛体の数
// @param　pairs ペア配列
// @param　numpairs ペア数
// @param　joints ジョイント配列
// @param　numJoints ジョイントア数
// @param　iteration 計算の反復回数
// @param　bias 位置補正のバイアス
// @param　slop 貫通許容誤差
// @param　timeStep タイムステップ
void solveConstraint(
	const State* states,
	const RigidBody* bodies,
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
