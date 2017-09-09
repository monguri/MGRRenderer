#pragma once

#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.hがmgrrendererにあるのでとりあえず。あとでそれらはCommon的なプロジェクトに移動する
using namespace mgrrenderer;

namespace mgrphysics
{
	Vec3 deltaLinearVelocity; // 並進速度差分
	Vec3 deltaAngularVelocity; // 回転速度差分
	Quaternion orientaion; // 姿勢
	Mat3 inertialInv; // 慣性テンソルの逆行列
	float massInv; // 質量の逆数
} // namespace mgrphysics
