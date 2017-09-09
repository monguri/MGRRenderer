#pragma once

// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.hがmgrrendererにあるのでとりあえず。あとでそれらはCommon的なプロジェクトに移動する
using namespace mgrrenderer;

namespace mgrphysics
{
struct State;
struct RigidBody;

// 拘束ソルバー
// @param　state 剛体の状態
// @param　body 剛体の属性
// @param　externalForce 与えるフォース
// @param　externalTorque 与えるトルク
// @param　timeStep タイムステップ
void applyExternalForce(
	State& state,
	RigidBody& body,
	const Vec3& externalForce,
	const Vec3& externalTorque,
	float timeStep
);

// 拘束ソルバー
// @param　states 剛体の状態の配列
// @param　numRigidBodies 剛体の数
// @param　timeStep タイムステップ
void integrate(
	State* states,
	unsigned int numRigidBodies,
	float timeStep
);
} // namespace mgrphysics
