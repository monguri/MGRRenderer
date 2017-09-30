#include "Contact.h"

namespace mgrphysics
{

int Contact::findNearestContactPoint(const Vec3 & newPointA, const Vec3 & newPointB, const Vec3 & newNormal)
{
	return -1;
}

int Contact::sort4ContactPoints(const Vec3 & newPoint, float newDistance)
{
	return 0;
}

void Contact::reset()
{
	numContacts = 0;
	for (int i = 0; i < NUM_CONTACTS; ++i)
	{
		contactPoints[i].reset();
	}
}

void Contact::removeContactPoint(int i)
{
	// 末尾の要素で上書きする
	contactPoints[i] = contactPoints[numContacts - 1];
	numContacts--;
}

void Contact::refresh(const Vec3 & pA, const Quaternion & qA, const Vec3 & pB, const Quaternion & qB)
{
	// 衝突点の閾値（平面方向）
	static const float CONTACT_THRESHOLD_TANGENT = 0.002f;

	// 衝突点の更新
	// 両衝突点の距離が閾値（CONTACT_THRESHOLD）を超えたら消去
	for (int i = 0; i < (int)numContacts; ++i)
	{
		const Vec3& normal = contactPoints[i].normal;
		// pointA、pointBについては前に計算に使ったものを用いて、剛体ABの位置とポーズの変更のみ反映する
		const Vec3& cpA = pA + Mat3::createRotation(qA) * contactPoints[i].pointA;
		const Vec3& cpB = pB + Mat3::createRotation(qB) * contactPoints[i].pointB;

		// 貫通深度がプラスに転じたかチェック（TODO:ここら辺、理論がよくわからんくなってきた）
		// 衝突点の閾値（法線方向）
		static const float CONTACT_THRESHOLD_NORMAL = 0.01f;
		float distanceNormalDir = Vec3::dot(normal, (cpA - cpB));
		if (distanceNormalDir > CONTACT_THRESHOLD_NORMAL)
		{
			removeContactPoint(i);
			i--;
			continue;
		}
		contactPoints[i].distance = distanceNormalDir;

		// 深度方向の距離を除去して平面上の距離をチェック
		const Vec3& cpAsubtractedNormal = cpA - normal * distanceNormalDir;
		float distanceOnPlane = (cpA - cpB).lengthSquare();
		// 衝突点の閾値（平面方向、2乗）
		static const float CONTACT_THRESHOLD_TANGENT = 0.002f;
		if (distanceOnPlane > CONTACT_THRESHOLD_TANGENT)
		{
			removeContactPoint(i);
			i--;
			continue;
		}
	}
}

void Contact::merge(const Contact & contact)
{
}

void Contact::addContact(float penetrationDepth, const Vec3 & normal, const Vec3 & contactPointA, const Vec3 & contactPointB)
{
}
} // namespace mgrphysics
