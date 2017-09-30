#pragma once

// TODO:�C���N���[�h�p�X��MGRRenderer/sources��ǉ����Ă���B���ƂŊO��
#include "renderer/BasicDataTypes.h"
// TODO:BasicDataTypes.h��mgrrenderer�ɂ���̂łƂ肠�����B���Ƃł�����Common�I�ȃv���W�F�N�g�Ɉړ�����
using namespace mgrrenderer;

namespace mgrphysics
{
struct ConvexMesh;

// 2�̓ʃ��b�V���̏Փˌ��o
// @param convexA �ʃ��b�V��A
// @param transformA A�̃��[���h�ϊ��s��
// @param convexB �ʃ��b�V��B
// @param transformB B�̃��[���h�ϊ��s��
// @param[out] normal �Փ˓_�̖@���x�N�g���i���[���h���W�n�j
// @param[out] penetrationDepth �ђʐ[�x
// @param[out] contactPointA �Փ˓_�i����A�̃��[�J�����W�n�j
// @param[out] contactPointB �Փ˓_�i����B�̃��[�J�����W�n�j
// @return�@�Փ˂����o�����ꍇ��true��Ԃ��B
bool convexConvexContact(
	const ConvexMesh& convexA,
	const Mat4& transformA,
	const ConvexMesh& convexB,
	const Mat4& transformB,
	Vec3& normal,
	float& penetrationDepth,
	Vec3& contactPointA,
	Vec3& contactPointB
);
} // namespace mgrphysics
