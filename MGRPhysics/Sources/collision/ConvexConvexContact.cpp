#include "ConvexConvexContact.h"
#include "../elements/ConvexMesh.h"
#include "../collision/ClosestFunction.h"

namespace mgrphysics
{
enum class SatType : int {
	PointAFacedB,
	PointBFacedA,
	EdgeEdge,
};

#define CHECK_MINMAX(axis, AMin, AMax, BMin, BMax, type) \
{ \
	satCount++; \
\
	float d1 = AMin - BMax; \
	float d2 = BMin - AMax; \
\
	if (d1 >= 0.0f || d2 >= 0.0f) \
	{ \
		return false; \
	} \
\
	if (distanceMin < d1) \
	{ \
		distanceMin = d1; \
		axisMin = axis; \
		satType = type; \
		axisFlip = false; \
	} \
\
	if (distanceMin < d2) \
	{ \
		distanceMin = d2; \
		axisMin = -axis; \
		satType = type; \
		axisFlip = true; \
	} \
}

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
	// Bローカル -> Aローカルへの変換
	// TODO:createInverse関数はMat4::createInverseのようにstatic関数にした方がいいな
	const Mat4& transformAB = transformA.createInverse() * transformB;
	const Vec3& offsetAB = transformAB.getTranslationVector();
	// Aローカル -> Bローカルへの変換
	const Mat4& transformBA = transformAB.createInverse();

	// 最も深い貫通深度とそのときの分離軸
	float distanceMin = -FLT_MAX;
	Vec3 axisMin(0.0f, 0.0f, 0.0f);
	SatType satType = SatType::EdgeEdge;
	bool axisFlip = false;

	// 分離軸判定
	int satCount = 0;
	{
		// ConvexAの面法線を分離軸とする
		for (unsigned int f = 0; f < convexA.numFacets; ++f)
		{
			const Facet& facet = convexA.facets[f];
			const Vec3& separatingAxis = facet.normal;

			// convexAを分離軸に投影
			float minA, maxA;
			convexA.GetProjection(minA, maxA, separatingAxis);

			// convexBを分離軸に投影
			float minB, maxB;
			convexB.GetProjection(minB, maxB, transformBA * separatingAxis);

			float offset = Vec3::dot(offsetAB, separatingAxis);
			minB += offset;
			maxB += offset;

			// 判定
			CHECK_MINMAX(separatingAxis, minA, maxA, minB, maxB, SatType::PointBFacedA);
		}

		// ConvexBの面法線を分離軸とする
		for (unsigned int f = 0; f < convexB.numFacets; ++f)
		{
			const Facet& facet = convexB.facets[f];
			const Vec3& separatingAxis = transformAB * facet.normal;

			// convexAを分離軸に投影
			float minA, maxA;
			convexA.GetProjection(minA, maxA, separatingAxis);

			// convexBを分離軸に投影
			float minB, maxB;
			convexB.GetProjection(minB, maxB, facet.normal);

			float offset = Vec3::dot(offsetAB, separatingAxis);
			minB += offset;
			maxB += offset;

			// 判定
			CHECK_MINMAX(separatingAxis, minA, maxA, minB, maxB, SatType::PointAFacedB);
		}

		// ConvexAとConvexBのエッジの外積を分離軸とする
		for (unsigned int eA = 0; eA < convexA.numEdges; ++eA)
		{
			const Edge& edgeA = convexA.edges[eA];
			if (edgeA.type != EdgeType::CONVEX)
			{
				continue;
			}

			const Vec3& edgeVecA = convexA.vertices[edgeA.vertId[1]] - convexA.vertices[edgeA.vertId[0]];

			for (unsigned int eB = 0; eB < convexB.numEdges; ++eB)
			{
				const Edge& edgeB = convexB.edges[eB];
				if (edgeB.type != EdgeType::CONVEX)
				{
					continue;
				}

				const Vec3& edgeVecB = convexA.vertices[edgeB.vertId[1]] - convexA.vertices[edgeB.vertId[0]];

				Vec3 separatingAxis = Vec3::cross(edgeVecA, edgeVecB);
				if (separatingAxis.lengthSquare() < 1e-5f * 1e-5f)
				{
					continue;
				}

				separatingAxis.normalize();

				// convexAを分離軸に投影
				float minA, maxA;
				convexA.GetProjection(minA, maxA, separatingAxis);

				// convexBを分離軸に投影
				float minB, maxB;
				convexB.GetProjection(minB, maxB, transformBA* separatingAxis);

				float offset = Vec3::dot(offsetAB, separatingAxis);
				minB += offset;
				maxB += offset;

				// 判定
				CHECK_MINMAX(separatingAxis, minA, maxA, minB, maxB, SatType::EdgeEdge);
			}
		}
	}

	// ここまで到達したので、２つの凸メッシュは交差している。
	// また、反発ベクトル(axisMin)と貫通深度(distanceMin)が求まった。
	// 反発ベクトルはＡを押しだす方向をプラスにとる。
	//Logger::log("sat check count %d\n", satCount);

	// 衝突座標検出
	{
		int collCount = 0;

		float closestMinSqr = FLT_MAX;
		Vec3 closestPointA, closestPointB;
		Vec3 separation = axisMin * 1.1f * fabs(distanceMin);

		for (unsigned int fA = 0; fA < convexA.numFacets; ++fA)
		{
			const Facet& facetA = convexA.facets[fA];

			float checkA = Vec3::dot(facetA.normal, -axisMin);
			if (satType == SatType::PointBFacedA && checkA < 0.99f && axisFlip)
			{
				// 判定軸が面Aの法線のとき、向きの違うAの面は判定しない
				continue;
			}

			if (checkA < 0.0f)
			{
				// 衝突面と逆に向いている面は判定しない
				continue;
			}

			for (unsigned int fB = 0; fB < convexB.numFacets; ++fB)
			{
				const Facet& facetB = convexB.facets[fB];

				float checkB = Vec3::dot(facetB.normal, -axisMin);
				if (satType == SatType::PointAFacedB && checkB < 0.99f && axisFlip)
				{
					// 判定軸が面Bの法線のとき、向きの違うBの面は判定しない
					continue;
				}

				if (checkB < 0.0f)
				{
					// 衝突面と逆に向いている面は判定しない
					continue;
				}

				collCount++;

				// 面Ａと面Ｂの最近接点を求める
				Vec3 triangleA[3] = {
					separation + convexA.vertices[facetA.vertId[0]],
					separation + convexA.vertices[facetA.vertId[1]],
					separation + convexA.vertices[facetA.vertId[2]]
				};

				Vec3 triangleB[3] = {
					separation + convexB.vertices[facetB.vertId[0]],
					separation + convexB.vertices[facetB.vertId[1]],
					separation + convexB.vertices[facetB.vertId[2]]
				};

				// エッジ同士の最近接点算出
				for (int i = 0; i < 3; ++i)
				{ 
					if (convexA.edges[facetA.edgeId[i]].type != EdgeType::CONVEX)
					{
						continue;
					}

					for (int j = 0; j < 3; ++j)
					{ 
						if (convexA.edges[facetB.edgeId[j]].type != EdgeType::CONVEX)
						{
							continue;
						}

						Vec3 sA, sB;
						getClosestTwoSegments(triangleA[i], triangleA[(i+1)%3], triangleB[j], triangleB[(j+1)%3], sA, sB);

						float dSqr = (sA - sB).lengthSquare();
						if (dSqr < closestMinSqr)
						{
							closestMinSqr = dSqr;
							closestPointA = sA;
							closestPointB = sB;
						}
					}
				}

				// 作業中！！！
				// 頂点A->面Bの最近接点算出
				for (int i = 0; i < 3; ++i)
				{ 
				}

				// 頂点B->面Aの最近接点算出
				for (int i = 0; i < 3; ++i)
				{ 
				}
			}
		}
	}

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
