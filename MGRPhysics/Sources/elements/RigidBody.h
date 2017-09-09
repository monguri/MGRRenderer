#pragma once

// TODO:�C���N���[�h�p�X��MGRRenderer/sources��ǉ����Ă���B���ƂŊO��
#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
using namespace mgrrenderer;

namespace mgrphysics
{
struct RigidBody
{
	// �����e���\��
	Mat3 inertia;
	// ����
	float mass;
	// �����W��
	float restitution;
	// ���C�W��
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
