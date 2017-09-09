#pragma once

// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.hがmgrrendererにあるのでとりあえず。あとでそれらはCommon的なプロジェクトに移動する
using namespace mgrrenderer;

namespace mgrphysics
{
enum class MotionType : int {
	ACTIVE = 0,
	STATIC,
};

struct State {
	Vec3 position;
	Quaternion orientation;
	Vec3 linearVelocity;
	Vec3 angularVelocity;
	MotionType motionType;

	void reset()
	{
		position = Vec3::ZERO;
		orientation = Quaternion::IDENTITY;
		linearVelocity = Vec3::ZERO;
		angularVelocity = Vec3::ZERO;
		motionType = MotionType::ACTIVE;
	}
};
} // namespace mgrphysics
