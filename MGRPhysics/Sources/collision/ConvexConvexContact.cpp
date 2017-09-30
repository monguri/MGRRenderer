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
	// B���[�J�� -> A���[�J���ւ̕ϊ�
	// TODO:createInverse�֐���Mat4::createInverse�̂悤��static�֐��ɂ�������������
	const Mat4& transformAB = transformA.createInverse() * transformB;
	const Vec3& offsetAB = transformAB.getTranslationVector();
	// A���[�J�� -> B���[�J���ւ̕ϊ�
	const Mat4& transformBA = transformAB.createInverse();

	// �ł��[���ђʐ[�x�Ƃ��̂Ƃ��̕�����
	float distanceMin = -FLT_MAX;
	Vec3 axisMin(0.0f, 0.0f, 0.0f);
	SatType satType = SatType::EdgeEdge;
	bool axisFlip = false;

	// ����������
	int satCount = 0;
	{
		// ConvexA�̖ʖ@���𕪗����Ƃ���
		for (unsigned int f = 0; f < convexA.numFacets; ++f)
		{
			const Facet& facet = convexA.facets[f];
			const Vec3& separatingAxis = facet.normal;

			// convexA�𕪗����ɓ��e
			float minA, maxA;
			convexA.GetProjection(minA, maxA, separatingAxis);

			// convexB�𕪗����ɓ��e
			float minB, maxB;
			convexB.GetProjection(minB, maxB, transformBA * separatingAxis);

			float offset = Vec3::dot(offsetAB, separatingAxis);
			minB += offset;
			maxB += offset;

			// ����
			CHECK_MINMAX(separatingAxis, minA, maxA, minB, maxB, SatType::PointBFacedA);
		}

		// ConvexB�̖ʖ@���𕪗����Ƃ���
		for (unsigned int f = 0; f < convexB.numFacets; ++f)
		{
			const Facet& facet = convexB.facets[f];
			const Vec3& separatingAxis = transformAB * facet.normal;

			// convexA�𕪗����ɓ��e
			float minA, maxA;
			convexA.GetProjection(minA, maxA, separatingAxis);

			// convexB�𕪗����ɓ��e
			float minB, maxB;
			convexB.GetProjection(minB, maxB, facet.normal);

			float offset = Vec3::dot(offsetAB, separatingAxis);
			minB += offset;
			maxB += offset;

			// ����
			CHECK_MINMAX(separatingAxis, minA, maxA, minB, maxB, SatType::PointAFacedB);
		}

		// ConvexA��ConvexB�̃G�b�W�̊O�ς𕪗����Ƃ���
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

				// convexA�𕪗����ɓ��e
				float minA, maxA;
				convexA.GetProjection(minA, maxA, separatingAxis);

				// convexB�𕪗����ɓ��e
				float minB, maxB;
				convexB.GetProjection(minB, maxB, transformBA* separatingAxis);

				float offset = Vec3::dot(offsetAB, separatingAxis);
				minB += offset;
				maxB += offset;

				// ����
				CHECK_MINMAX(separatingAxis, minA, maxA, minB, maxB, SatType::EdgeEdge);
			}
		}
	}

	// �����܂œ��B�����̂ŁA�Q�̓ʃ��b�V���͌������Ă���B
	// �܂��A�����x�N�g��(axisMin)�Ɗђʐ[�x(distanceMin)�����܂����B
	// �����x�N�g���͂`�����������������v���X�ɂƂ�B
	//Logger::log("sat check count %d\n", satCount);

	// �Փˍ��W���o
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
				// ���莲����A�̖@���̂Ƃ��A�����̈ႤA�̖ʂ͔��肵�Ȃ�
				continue;
			}

			if (checkA < 0.0f)
			{
				// �Փ˖ʂƋt�Ɍ����Ă���ʂ͔��肵�Ȃ�
				continue;
			}

			for (unsigned int fB = 0; fB < convexB.numFacets; ++fB)
			{
				const Facet& facetB = convexB.facets[fB];

				float checkB = Vec3::dot(facetB.normal, -axisMin);
				if (satType == SatType::PointAFacedB && checkB < 0.99f && axisFlip)
				{
					// ���莲����B�̖@���̂Ƃ��A�����̈ႤB�̖ʂ͔��肵�Ȃ�
					continue;
				}

				if (checkB < 0.0f)
				{
					// �Փ˖ʂƋt�Ɍ����Ă���ʂ͔��肵�Ȃ�
					continue;
				}

				collCount++;

				// �ʂ`�Ɩʂa�̍ŋߐړ_�����߂�
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

				// �G�b�W���m�̍ŋߐړ_�Z�o
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

				// ��ƒ��I�I�I
				// ���_A->��B�̍ŋߐړ_�Z�o
				for (int i = 0; i < 3; ++i)
				{ 
				}

				// ���_B->��A�̍ŋߐړ_�Z�o
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
