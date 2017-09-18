#include "ConvexConvexContact.h"
#include "../elements/ConvexMesh.h"

namespace mgrphysics
{
bool convexConvexContact_local(
	const ConvexMesh& convexA,
	const Mat4& transformA,
	const ConvexMesh& convexB,
	const Mat4& transformB,
	Vec3& normal,
	float& penetrationDepth,
	Vec3& contactPointA,
	Vec3& contactPointB
)
{
	Mat4 transformAB, transformBA;
	// B���[�J�� -> A���[�J���ւ̕ϊ�
	// TODO:createInverse�֐���Mat4::createInverse�̂悤��static�֐��ɂ�������������
	const Mat4& transformAB = transformA.createInverse() * transformB;
	// A���[�J�� -> B���[�J���ւ̕ϊ�
	const Mat4& transformBA = transformAB.createInverse();

	// �ł��[���ђʐ[�x�Ƃ��̂Ƃ��̕�����
	float distanceMin = -FLT_MAX;
	Vec3 axisMin(0.0f, 0.0f, 0.0f);

	// ��ƒ��I�I�I
	return true;
}

bool convexConvexContact(
	const ConvexMesh& convexA,
	const Mat4& transformA,
	const ConvexMesh& convexB,
	const Mat4& transformB,
	Vec3& normal,
	float& penetrationDepth,
	Vec3& contactPointA,
	Vec3& contactPointB
)
{
	bool ret = false;
	// ���W�n�ϊ��̉񐔂����炷���߁A�ʐ��̑����������W�n�̊�ɂƂ�
	if (convexA.numFacets >= convexB.numFacets)
	{
		ret = convexConvexContact_local(
			convexA,
			transformA,
			convexB,
			transformB,
			normal,
			penetrationDepth,
			contactPointA,
			contactPointB
		);
	}
	else
	{
		ret = convexConvexContact_local(
			convexB,
			transformB,
			convexA,
			transformA,
			normal,
			penetrationDepth,
			contactPointB,
			contactPointA
		);
		normal *= -1;
	}

	return ret;
}
} // namespace mgrphysics
