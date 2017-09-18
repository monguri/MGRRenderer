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
	// Bローカル -> Aローカルへの変換
	// TODO:createInverse関数はMat4::createInverseのようにstatic関数にした方がいいな
	const Mat4& transformAB = transformA.createInverse() * transformB;
	// Aローカル -> Bローカルへの変換
	const Mat4& transformBA = transformAB.createInverse();

	// 最も深い貫通深度とそのときの分離軸
	float distanceMin = -FLT_MAX;
	Vec3 axisMin(0.0f, 0.0f, 0.0f);

	// 作業中！！！
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
	// 座標系変換の回数を減らすため、面数の多い方を座標系の基準にとる
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
