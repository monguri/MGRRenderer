#pragma once

// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.hがmgrrendererにあるのでとりあえず。あとでそれらはCommon的なプロジェクトに移動する
using namespace mgrrenderer;

namespace mgrphysics
{
// 衝突点
struct Constraint
{
	// 拘束軸
	Vec3 axis;
	// 拘束式の分母
	float jacDiagInv;
	// 初期拘束力
	float rhs;
	// 拘束力の下限
	float lowerLimit;
	// 拘束力の上限
	float upperLimit;
	// 蓄積される拘束力
	float accumImpulse;
};
} // namespace mgrphysics
