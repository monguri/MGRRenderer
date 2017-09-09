#pragma once

// TODO:�C���N���[�h�p�X��MGRRenderer/sources��ǉ����Ă���B���ƂŊO��
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
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
