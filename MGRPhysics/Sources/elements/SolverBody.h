#pragma once

#include "renderer/BasicDataTypes.h"

// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
using namespace mgrrenderer;

namespace mgrphysics
{
	Vec3 deltaLinearVelocity; // ���i���x����
	Vec3 deltaAngularVelocity; // ��]���x����
	Quaternion orientaion; // �p��
	Mat3 inertialInv; // �����e���\���̋t�s��
	float massInv; // ���ʂ̋t��
} // namespace mgrphysics
