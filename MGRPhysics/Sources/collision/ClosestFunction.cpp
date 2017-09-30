#include "ClosestFunction.h"

struct Vec3;

namespace mgrphysics
{
/// �Q�̐����̍ŋߐړ_���o
/// @param segmentPointA0 ����A�̎n�_
/// @param segmentPointA1 ����A�̏I�_
/// @param segmentPointB0 ����B�̎n�_
/// @param segmentPointB1 ����B�̏I�_
/// @param[out] closestPointA ����A��̍ŋߐړ_
/// @param[out] closestPointB ����B��̍ŋߐړ_
void getClosestTwoSegments(const Vec3& segmentPointA0, const Vec3& segmentPointA1, const Vec3& segmentPointB0, const Vec3& segmentPointB1, Vec3& closestPointA, Vec3& closestPointB);

/// ���_����R�p�`�ʂւ̍ŋߐړ_���o
/// @param point ���_
/// @param trianglePoint0 �R�p�`�ʂ̒��_0
/// @param trianglePoint1 �R�p�`�ʂ̒��_1
/// @param trianglePoint2 �R�p�`�ʂ̒��_2
/// @param triangleNormal �R�p�`�ʂ̖@���x�N�g��
/// @param[out] closestPoint �R�p�`�ʏ�̍ŋߐړ_
void getClosestPointTriangle(const Vec3& point, const Vec3& trianglePoint0, const Vec3& trianglePoint1, const Vec3& trianglePoint2, const Vec3& triangleNormal, Vec3& closestPoint);
} // namespace mgrphysics
