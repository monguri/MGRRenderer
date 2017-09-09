#pragma once

// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "renderer/BasicDataTypes.h"
#include "Constraint.h"

// TODO:BasicDataTypes.hがmgrrendererにあるのでとりあえず。あとでそれらはCommon的なプロジェクトに移動する
using namespace mgrrenderer;

namespace mgrphysics
{
struct BallJoint
{
	// 拘束の強さの調整値
	float bias;
	// 剛体Aのインデックス
	unsigned int rigidBodyA;
	// 剛体Bのインデックス
	unsigned int rigidBodyB;
	// 剛体Aのローカル座標における接続点
	Vec3 anchorA;
	// 剛体Bのローカル座標における接続点
	Vec3 anchorB;
	// 拘束
	Constraint constraint;

	void reset()
	{
		bias = 0.1f;
		constraint.accumImpulse = 0.0f;
	}
};
} // namespace mgrphysics
