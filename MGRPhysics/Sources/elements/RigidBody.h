#pragma once

// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.hがmgrrendererにあるのでとりあえず。あとでそれらはCommon的なプロジェクトに移動する
using namespace mgrrenderer;

namespace mgrphysics
{
struct RigidBody
{
	// 慣性テンソル
	Mat3 inertia;
	// 質量
	float mass;
	// 反発係数
	float restitution;
	// 摩擦係数
	float friction;

	void reset()
	{
		mass = 1.0f;
		inertia = Mat3::IDENTITY;
		restitution = 0.2f;
		friction = 0.6f;
	}
};
} // namespace mgrphysics
